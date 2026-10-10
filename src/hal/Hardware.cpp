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
    : control_(config::kControlDebounceMs, config::kControlHoldMs, config::kControlDoubleClickMs),
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
    cfg.internal_imu = config::kImuEnabled;      // default true: accel + gyro on (~0.7 mA)
    cfg.output_power = config::kGrovePortPower;  // default true: Grove 5 V boost on
    M5.begin(cfg);  // must come before any M5.Display call (M5Unified 0.2.25 fix)
    Serial.begin(115200);
    if (!config::kImuEnabled) suspendImu();

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
    // M5GFX switches the LED on during board detection; set our awake state.
    // (Also turns it off again after a deep-sleep wake.)
    setStatusLed(config::kStatusLedWhenAwake);
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
    if (c & ButtonLogic::Click) fifo_.push({InputType::ControlClick, nowMicros(), 0, 0});
    if (c & ButtonLogic::Hold) fifo_.push({InputType::ControlHold, nowMicros(), 0, 0});
    if (c & ButtonLogic::DoubleClick) {
        fifo_.push({InputType::ControlDoubleClick, nowMicros(), 0, 0});
    }

    if (!config::kUseInterruptTiming) {
        const uint8_t t = timingPoll_.update(nowMs, readPressed(config::kPinTimingButton));
        if (t & ButtonLogic::Pressed) fifo_.push({InputType::TimingTap, nowMicros(), 0, 0});
    }

    pollTouch();

    if (nowMs - lastBatteryPollMs_ >= config::kBatteryPollMs) {
        lastBatteryPollMs_ = nowMs;
        readBattery();
    }
}

bool Hardware::pollEvent(InputEvent& out) {
    if (gTapQueue) {
        Micros t;
        if (xQueueReceive(gTapQueue, &t, 0) == pdTRUE) {
            out = {InputType::TimingTap, t, 0, 0};
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

void Hardware::pollTouch() {
    // M5.update() already read the CST820. Report a tap when a finger lifts
    // without having dragged, and a drag sample whenever a held finger moves
    // (or first lands) -- the slider follows those.
    if (!M5.Touch.isEnabled()) return;
    const auto t = M5.Touch.getDetail();
    if (t.wasClicked()) fifo_.push({InputType::TouchTap, nowMicros(), t.x, t.y});
    if (t.isPressed()) {
        if (t.wasPressed() || t.x != lastTouchX_ || t.y != lastTouchY_) {
            fifo_.push({InputType::TouchDrag, nowMicros(), t.x, t.y});
            lastTouchX_ = t.x;
            lastTouchY_ = t.y;
        }
    } else {
        lastTouchX_ = lastTouchY_ = -1;
    }
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

void Hardware::suspendImu() {
    // BMI270 at 0x68 on the internal bus. It sits on the always-on L1 rail, so a
    // reboot (including a deep-sleep wake) doesn't reset it: whatever mode an
    // earlier firmware left it in persists. Put it in suspend explicitly.
    constexpr uint8_t kAddr = 0x68;
    constexpr uint8_t kPwrConf = 0x7C;  // bit0 = advanced power save
    constexpr uint8_t kPwrCtrl = 0x7D;  // bits: aux, gyro, accel, temp enable
    constexpr uint32_t kFreq = 400000;
    const bool ok = M5.In_I2C.writeRegister8(kAddr, kPwrCtrl, 0x00, kFreq) &&  // all sensors off
                    M5.In_I2C.writeRegister8(kAddr, kPwrConf, 0x01, kFreq);    // power save on
    if (!ok) Serial.println("[hw] IMU suspend failed (no BMI270 at 0x68?)");
}

void Hardware::setStatusLed(bool on) {
    if (M5.Power.getType() != m5::Power_Class::pmic_m5pm1) return;
    M5.Power.M5pm1.setLedEnLevel(on);  // PWR_CFG bit 4 (LED_EN) -> green LED1
}

Hardware::WakeResult Hardware::lightSleep(Millis maxDurationMs) {
    using config::LedMode;
    const LedMode led = config::kStatusLedLightSleep;

    M5.Power.setVibration(0);
    M5.Display.sleep();  // brightness 0 + panel sleep-in (AMOLED draws ~nothing)
    M5.Display.waitDisplay();
    setStatusLed(led == LedMode::On);

    // GPIO wake-up reprograms the pin's interrupt type to "level", which would
    // fight our edge interrupt -- so take the ISR off for the duration.
    detachTimingInterrupt();
    const gpio_int_type_t pressed =
        config::kButtonsActiveLow ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL;
    gpio_wakeup_enable(kTimingGpio, pressed);
    gpio_wakeup_enable(kControlGpio, pressed);
    esp_sleep_enable_gpio_wakeup();

    Serial.println("[power] light sleep");
    Serial.flush();

    // Sleep in slices: each timer wake-up either blinks the LED (a few ms
    // awake, then asleep again with it lit for kStatusLedFlashOnMs) or ends
    // the light sleep because deep sleep is due. Only a button, the deadline,
    // or a rejected sleep returns to the caller.
    const Micros startUs = nowMicros();
    const Micros budgetUs = static_cast<Micros>(maxDurationMs) * 1000ULL;  // 0 = no deadline
    const Micros flashPeriodUs = static_cast<Micros>(config::kStatusLedFlashPeriodMs) * 1000ULL;
    const Micros flashOnUs = static_cast<Micros>(config::kStatusLedFlashOnMs) * 1000ULL;

    WakeReason reason = WakeReason::Other;
    Micros wokeAt = 0;
    bool timingDown = false;
    bool controlDown = false;

    auto sleepFor = [&](Micros us) -> bool {  // false = sleep was rejected
        if (us) {
            esp_sleep_enable_timer_wakeup(us);
        } else {
            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
        }
        const bool ok = esp_light_sleep_start() == ESP_OK;
        wokeAt = nowMicros();
        timingDown = readPressed(config::kPinTimingButton);
        controlDown = readPressed(config::kPinControlButton);
        return ok;
    };

    for (;;) {
        // Next timer wake-up: the next flash or the deadline, whichever comes first.
        Micros sliceUs = (led == LedMode::Flash) ? flashPeriodUs : 0;
        if (budgetUs) {
            const Micros elapsed = nowMicros() - startUs;
            if (elapsed >= budgetUs) {
                reason = WakeReason::Timer;
                break;
            }
            const Micros left = budgetUs - elapsed;
            if (!sliceUs || left < sliceUs) sliceUs = left;
        }

        if (!sleepFor(sliceUs)) break;         // rejected (e.g. button held): Other
        if (timingDown || controlDown) break;  // a button: handled below
        if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_TIMER) continue;  // spurious
        if (budgetUs && nowMicros() - startUs >= budgetUs) {
            reason = WakeReason::Timer;
            break;
        }

        if (led == LedMode::Flash) {
            setStatusLed(true);
            const bool ok = sleepFor(flashOnUs);  // stay asleep while it's lit
            setStatusLed(false);
            if (!ok || timingDown || controlDown) break;
        }
    }

    setStatusLed(false);
    gpio_wakeup_disable(kTimingGpio);
    gpio_wakeup_disable(kControlGpio);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    attachTimingInterrupt();

    // The wake press must not ALSO act as a click (or a polled tap) later.
    if (controlDown) control_.suppressUntilRelease();
    if (timingDown) timingPoll_.suppressUntilRelease();

    // Buttons first: a press can race the timer.
    if (timingDown) {
        reason = WakeReason::TimingButton;
    } else if (controlDown) {
        reason = WakeReason::ControlButton;
    }
    Serial.printf("[power] woke: %s\n", reason == WakeReason::Timer           ? "timer"
                                        : reason == WakeReason::TimingButton  ? "yellow"
                                        : reason == WakeReason::ControlButton ? "blue"
                                                                              : "other");
    return {reason, wokeAt};
}

void Hardware::wakeUi() {
    M5.Display.wakeup();
    setStatusLed(config::kStatusLedWhenAwake);
}

void Hardware::deepSleep() {
    M5.Power.setVibration(0);
    M5.Speaker.end();
    M5.Display.sleep();
    M5.Display.waitDisplay();
    setStatusLed(config::kStatusLedInDeepSleep);  // the M5PM1 holds it through deep sleep
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
