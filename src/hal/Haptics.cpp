#include "Haptics.h"

#include <M5Unified.h>

#include "../Config.h"

namespace curl {

using namespace config;

void Haptics::tap(Millis now) {
    const Step s[] = {{kHapticTapMs, 0}};
    play(s, 1, now);
}

void Haptics::result(Millis now) {
    const Step s[] = {{kHapticResultMs, kHapticGapMs}, {kHapticResultMs, 0}};
    play(s, 2, now);
}

void Haptics::cancel(Millis now) {
    const Step s[] = {{kHapticCancelMs, 0}};
    play(s, 1, now);
}

void Haptics::play(const Step* steps, uint8_t count, Millis now) {
    if (count > kMaxSteps) count = kMaxSteps;
    for (uint8_t i = 0; i < count; ++i) steps_[i] = steps[i];
    count_ = count;
    index_ = 0;
    active_ = count > 0;
    phaseStart_ = now;
    motor(active_);
}

void Haptics::update(Millis now) {
    if (!active_) return;
    const Step& s = steps_[index_];
    const Millis elapsed = now - phaseStart_;

    if (motorOn_ && elapsed >= s.onMs) {
        motor(false);
        phaseStart_ = now;
        if (s.offMs == 0) {  // no gap: go straight to the next step (or finish)
            if (++index_ >= count_) {
                active_ = false;
                return;
            }
            motor(true);
        }
    } else if (!motorOn_ && elapsed >= s.offMs) {
        if (++index_ >= count_) {
            active_ = false;
            return;
        }
        phaseStart_ = now;
        motor(true);
    }
}

void Haptics::stop() {
    active_ = false;
    motor(false);
}

void Haptics::motor(bool on) {
    if (on == motorOn_) return;  // avoid redundant I2C writes to the IO expander
    motorOn_ = on;
    M5.Power.setVibration(on ? kHapticStrength : 0);
}

}  // namespace curl
