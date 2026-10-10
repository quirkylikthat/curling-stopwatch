#include "Hardware.h"

#include <M5Unified.h>
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

}  // namespace

Hardware::Hardware()
    : control_(config::kControlDebounceMs, config::kControlHoldMs),
      timingPoll_(config::kControlDebounceMs, config::kControlHoldMs) {}

bool Hardware::begin() {
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
        attachInterrupt(digitalPinToInterrupt(config::kPinTimingButton), onTimingEdge,
                        config::kButtonsActiveLow ? FALLING : RISING);
    }

    M5.Speaker.setVolume(config::kSpeakerVolume);
    M5.Power.setVibration(0);
    readBattery();
    lastBatteryPollMs_ = nowMillis();

    Serial.printf("[hw] board=%d timing=%s idle levels: yellow=%d blue=%d\n",
                  static_cast<int>(M5.getBoard()), config::kUseInterruptTiming ? "isr" : "poll",
                  digitalRead(config::kPinTimingButton), digitalRead(config::kPinControlButton));
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
}

}  // namespace curl
