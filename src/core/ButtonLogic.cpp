#include "ButtonLogic.h"

namespace curl {

uint8_t ButtonLogic::update(Millis now, bool rawPressed) {
    uint8_t events = None;

    if (rawPressed != lastRaw_) {
        lastRaw_ = rawPressed;
        rawChangedAt_ = now;
    }

    // Unsigned subtraction is wrap-safe for millis().
    if (rawPressed != stable_ && static_cast<Millis>(now - rawChangedAt_) >= debounceMs_) {
        stable_ = rawPressed;
        if (stable_) {
            events |= Pressed;
            pressedAt_ = rawChangedAt_;
            holdFired_ = false;
        } else {
            events |= Released;
            if (!holdFired_) events |= Click;
        }
    }

    if (stable_ && !holdFired_ && static_cast<Millis>(now - pressedAt_) >= holdMs_) {
        holdFired_ = true;
        events |= Hold;
    }

    return events;
}

}  // namespace curl
