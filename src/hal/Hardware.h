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

private:
    void readBattery();

    ButtonLogic control_;
    ButtonLogic timingPoll_;  // only used when kUseInterruptTiming == false
    EventFifo<8> fifo_;

    int8_t batteryPercent_ = -1;
    bool charging_ = false;
    Millis lastBatteryPollMs_ = 0;
};

}  // namespace curl
