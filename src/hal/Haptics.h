// Haptics: non-blocking vibration patterns.
//
// Never use delay() for feedback -- it would stall the main loop. Instead a
// pattern is a list of on/off steps advanced by update().
#pragma once

#include "../core/Types.h"

namespace curl {

class Haptics {
public:
    void tap(Millis now);     // short buzz: tap accepted
    void result(Millis now);  // double buzz: measurement ready
    void cancel(Millis now);  // long buzz: cancelled / timed out
    void update(Millis now);
    void stop();

private:
    struct Step {
        Millis onMs;
        Millis offMs;
    };
    static constexpr uint8_t kMaxSteps = 4;

    void play(const Step* steps, uint8_t count, Millis now);
    void motor(bool on);

    Step steps_[kMaxSteps] = {};
    uint8_t count_ = 0;
    uint8_t index_ = 0;
    bool active_ = false;
    bool motorOn_ = false;
    Millis phaseStart_ = 0;
};

}  // namespace curl
