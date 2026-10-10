// Hardware: converts the physical device into InputEvents, clocks and battery
// readings. Along with Haptics, Settings and Renderer, this is the only code
// that talks to M5Unified / ESP32 APIs.
//
// Timing button (yellow): a GPIO interrupt timestamps the press edge with
// esp_timer_get_time() and pushes it to a FreeRTOS queue. The main loop can
// spend 20-30 ms drawing a frame without adding any error to the timing.
#pragma once

#include "../core/ButtonLogic.h"
#include "../core/EventFifo.h"
#include "../core/Types.h"

namespace curl {

class Hardware {
public:
    Hardware();

    bool begin();
    void update();  // call once per loop()

    // Drain input events (interrupt-captured taps first, then polled events).
    bool pollEvent(InputEvent& out);

    Micros nowMicros() const;
    Millis nowMillis() const;

    int8_t batteryPercent() const { return batteryPercent_; }  // -1 = unknown
    bool isCharging() const { return charging_; }

    void setBrightness(uint8_t level);
    void beep();

    // --- Power -----------------------------------------------------------
    bool onUsbPower() const { return usbPower_; }
    bool wokeFromDeepSleep() const { return wokeFromDeepSleep_; }

    enum class WakeReason : uint8_t { Timer, TimingButton, ControlButton, Other };
    struct WakeResult {
        WakeReason reason;
        Micros wokeAt;  // esp_timer time right after the CPU resumed
    };

    // Screen off, CPU paused, RAM kept. Returns when a button is pressed or
    // after maxDurationMs (0 = no timer). While asleep the status LED follows
    // kStatusLedLightSleep (Flash = brief wake-ups every kStatusLedFlashPeriodMs
    // to blink it; they don't return to the caller). The screen stays OFF on
    // return; call wakeUi() unless you're heading straight into deep sleep.
    WakeResult lightSleep(Millis maxDurationMs);

    // Screen back on, status LED back to its "awake" state.
    void wakeUi();

    // Everything except RTC memory off; the status LED is set to
    // kStatusLedInDeepSleep (the power chip holds it). Either button reboots
    // the device. Does not return.
    [[noreturn]] void deepSleep();

    // Green status LED on the M5PM1 (no-op on any other power chip).
    void setStatusLed(bool on);

private:
    void readBattery();
    void attachTimingInterrupt();
    void detachTimingInterrupt();
    void suspendImu();
    void pollTouch();

    int16_t lastTouchX_ = -1;
    int16_t lastTouchY_ = -1;
    int16_t touchDownX_ = -1;  // where the current touch landed
    int16_t touchDownY_ = -1;
    int touchMaxMove_ = 0;  // furthest it has strayed from there, px

    bool usbPower_ = false;
    bool wokeFromDeepSleep_ = false;

    ButtonLogic control_;
    ButtonLogic timingPoll_;  // only used when kUseInterruptTiming == false
    EventFifo<8> fifo_;

    int8_t batteryPercent_ = -1;
    bool charging_ = false;
    Millis lastBatteryPollMs_ = 0;
};

}  // namespace curl
