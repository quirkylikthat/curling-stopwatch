#include "Hardware.h"

#include <M5Unified.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <esp_timer.h>

#include "../Config.h"

namespace curl {

namespace {

QueueHandle_t gTapQueue = nullptr;
volatile Micros gLastEdgeUs = 0;

// Runs in interrupt context: keep it tiny. No Serial, no M5 calls, no heap.
void ARDUINO_ISR_ATTR onTimingEdge() {
    const Micros t = static_cast<Micros>(esp_timer_get_time());
    if (t - gLastEdgeUs < config::kTimingIsrLockoutUs) return;  // contact bounce
    gLastEdgeUs = t;
    xQueueSendFromISR(gTapQueue, &t, nullptr);  // drops the tap if the queue is full
}

bool readPressed(int pin) {
    const bool high = digitalRead(pin) == HIGH;
    return config::kButtonsActiveLow ? !high : high;
}

constexpr gpio_num_t kTimingGpio = static_cast<gpio_num_t>(config::kPinTimingButton);
constexpr gpio_num_t kControlGpio = static_cast<gpio_num_t>(config::kPinControlButton);

}  // namespace

Hardware::Hardware()
    : control_(config::kControlDebounceMs, config::kControlHoldMs),
      timingPoll_(config::kControlDebounceMs, config::kControlHoldMs) {}

bool Hardware::begin() {
    // Waking from deep sleep is a reboot. The wake-up pins are still routed to
    // the RTC domain at this point and must be handed back to normal GPIO.
    wokeFromDeepSleep_ = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1;
    if (wokeFromDeepSleep_) {
        rtc_gpio_deinit(kTimingGpio);
        rtc_gpio_deinit(kControlGpio);
    }

    auto cfg = M5.config();
    M5.begin(cfg);  // must come before any M5.Display call (M5Unified 0.2.25 fix)
    Serial.begin(115200);

    // Same as M5Unified does for this board: plain inputs, external pull-ups.
    // (M5Unified only polls these pins, so our interrupt doesn't conflict.)
    pinMode(config::kPinTimingButton, INPUT);
    pinMode(config::kPinControlButton, INPUT);

    if (config::kUseInterruptTiming) {
        gTapQueue = xQueueCreate(8, sizeof(Micros));
        if (!gTapQueue) {
            Serial.println("[hw] tap queue alloc failed");
            return false;
        }
        attachTimingInterrupt();
    }

    // A button still held from the deep-sleep wake (or at power-on) must not
    // turn into a click when it's released.
    if (readPressed(config::kPinControlButton)) control_.suppressUntilRelease();
    if (readPressed(config::kPinTimingButton)) timingPoll_.suppressUntilRelease();

    M5.Speaker.setVolume(config::kSpeakerVolume);
    M5.Power.setVibration(0);
    readBattery();
    lastBatteryPollMs_ = nowMillis();

    Serial.printf("[hw] board=%d timing=%s idle levels: yellow=%d blue=%d wake=%s usb=%d\n",
                  static_cast<int>(M5.getBoard()), config::kUseInterruptTiming ? "isr" : "poll",
                  digitalRead(config::kPinTimingButton), digitalRead(config::kPinControlButton),
                  wokeFromDeepSleep_ ? "deep-sleep" : "power-on", usbPower_ ? 1 : 0);
    return true;
}

void Hardware::update() {
    M5.update();  // touch, power key etc. (we read the two buttons ourselves)

    const Millis nowMs = nowMillis();

    const uint8_t c = control_.update(nowMs, readPressed(config::kPinControlButton));
    if (c & ButtonLogic::Click) fifo_.push({InputType::ControlClick, nowMicros()});
    if (c & ButtonLogic::Hold) fifo_.push({InputType::ControlHold, nowMicros()});

    if (!config::kUseInterruptTiming) {
        const uint8_t t = timingPoll_.update(nowMs, readPressed(config::kPinTimingButton));
        if (t & ButtonLogic::Pressed) fifo_.push({InputType::TimingTap, nowMicros()});
    }

    if (nowMs - lastBatteryPollMs_ >= config::kBatteryPollMs) {
        lastBatteryPollMs_ = nowMs;
        readBattery();
    }
}

bool Hardware::pollEvent(InputEvent& out) {
    if (gTapQueue) {
        Micros t;
        if (xQueueReceive(gTapQueue, &t, 0) == pdTRUE) {
            out = {InputType::TimingTap, t};
            return true;
        }
    }
    return fifo_.pop(out);
}

Micros Hardware::nowMicros() const { return static_cast<Micros>(esp_timer_get_time()); }

Millis Hardware::nowMillis() const { return static_cast<Millis>(millis()); }

void Hardware::setBrightness(uint8_t level) { M5.Display.setBrightness(level); }

void Hardware::beep() {
    if (config::kBeepEnabled) M5.Speaker.tone(config::kBeepHz, config::kBeepMs);
}

void Hardware::readBattery() {
    const int32_t level = M5.Power.getBatteryLevel();
    batteryPercent_ = (level < 0 || level > 100) ? -1 : static_cast<int8_t>(level);
    // isCharging() returns an enum; "unknown" must not read as charging.
    charging_ = M5.Power.isCharging() == m5::Power_Class::is_charging;
    // VBUS in mV from the M5PM1, or -1 if it can't be read (-> treat as battery).
    usbPower_ = M5.Power.getVBUSVoltage() >= config::kUsbPresentMillivolts;
}

// --- Power -------------------------------------------------------------------

void Hardware::attachTimingInterrupt() {
    if (!config::kUseInterruptTiming) return;
    attachInterrupt(digitalPinToInterrupt(config::kPinTimingButton), onTimingEdge,
                    config::kButtonsActiveLow ? FALLING : RISING);
}

void Hardware::detachTimingInterrupt() {
    if (!config::kUseInterruptTiming) return;
    detachInterrupt(digitalPinToInterrupt(config::kPinTimingButton));
}

Hardware::WakeResult Hardware::lightSleep(Millis maxDurationMs) {
    M5.Power.setVibration(0);
    M5.Display.sleep();  // brightness 0 + panel sleep-in (AMOLED draws ~nothing)
    M5.Display.waitDisplay();

    // GPIO wake-up reprograms the pin's interrupt type to "level", which would
    // fight our edge interrupt -- so take the ISR off for the duration.
    detachTimingInterrupt();
    const gpio_int_type_t pressed =
        config::kButtonsActiveLow ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL;
    gpio_wakeup_enable(kTimingGpio, pressed);
    gpio_wakeup_enable(kControlGpio, pressed);
    esp_sleep_enable_gpio_wakeup();
    if (maxDurationMs)
        esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(maxDurationMs) * 1000ULL);

    Serial.println("[power] light sleep");
    Serial.flush();
    esp_light_sleep_start();  // returns at once if a button is already held
    const Micros wokeAt = nowMicros();

    const esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    const bool timingDown = readPressed(config::kPinTimingButton);
    const bool controlDown = readPressed(config::kPinControlButton);

    gpio_wakeup_disable(kTimingGpio);
    gpio_wakeup_disable(kControlGpio);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    attachTimingInterrupt();

    // The wake press must not ALSO act as a click (or a polled tap) later.
    if (controlDown) control_.suppressUntilRelease();
    if (timingDown) timingPoll_.suppressUntilRelease();

    // Buttons first: a press can race the timer.
    WakeReason reason = WakeReason::Other;
    if (timingDown) {
        reason = WakeReason::TimingButton;
    } else if (controlDown) {
        reason = WakeReason::ControlButton;
    } else if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        reason = WakeReason::Timer;
    }
    Serial.printf("[power] woke: %s\n", reason == WakeReason::Timer           ? "timer"
                                        : reason == WakeReason::TimingButton  ? "yellow"
                                        : reason == WakeReason::ControlButton ? "blue"
                                                                              : "other");
    return {reason, wokeAt};
}

void Hardware::wakeDisplay() { M5.Display.wakeup(); }

void Hardware::deepSleep() {
    M5.Power.setVibration(0);
    M5.Speaker.end();
    M5.Display.sleep();
    M5.Display.waitDisplay();
    detachTimingInterrupt();

    // A held button would wake us immediately; give it a moment to be released.
    const Millis start = nowMillis();
    while ((readPressed(config::kPinTimingButton) || readPressed(config::kPinControlButton)) &&
           nowMillis() - start < 2000) {
        delay(10);
    }

    // Either button wakes (both are RTC-capable GPIOs on the ESP32-S3). The
    // board's external pull-ups stay powered, so no internal pulls are needed.
    const uint64_t mask = (1ULL << config::kPinTimingButton) | (1ULL << config::kPinControlButton);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_sleep_enable_ext1_wakeup_io(
        mask, config::kButtonsActiveLow ? ESP_EXT1_WAKEUP_ANY_LOW : ESP_EXT1_WAKEUP_ANY_HIGH);

    Serial.println("[power] deep sleep");
    Serial.flush();
    esp_deep_sleep_start();
}

}  // namespace curl
