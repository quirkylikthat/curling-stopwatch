#include "ButtonLogic.h"

namespace curl {

uint8_t ButtonLogic::update(Millis now, bool rawPressed) {
    uint8_t events = None;

    if (rawPressed != lastRaw_) {
        lastRaw_ = rawPressed;
        rawChangedAt_ = now;
    }

    if (suppress_) {
        // Wait for a debounced release, then resume from a clean "up" state.
        if (!rawPressed && static_cast<Millis>(now - rawChangedAt_) >= debounceMs_) {
            suppress_ = false;
            stable_ = false;
            holdFired_ = false;
        }
        return None;
    }

    // Unsigned subtraction is wrap-safe for millis().
    if (rawPressed != stable_ && static_cast<Millis>(now - rawChangedAt_) >= debounceMs_) {
        stable_ = rawPressed;
        if (stable_) {
            events |= Pressed;
            pressedAt_ = rawChangedAt_;
            holdFired_ = false;
            if (pendingClick_) {
                if (static_cast<Millis>(rawChangedAt_ - pendingSince_) < doubleClickMs_) {
                    secondPress_ = true;  // may become a double-click
                } else {
                    // Too late to pair up (loop was busy): the first one was a single click.
                    events |= Click;
                    pendingClick_ = false;
                }
            }
        } else {
            events |= Released;
            if (!holdFired_) {
                if (doubleClickMs_ == 0) {
                    events |= Click;
                } else if (secondPress_) {
                    events |= DoubleClick;
                    pendingClick_ = false;
                    secondPress_ = false;
                } else {
                    pendingClick_ = true;
                    pendingSince_ = rawChangedAt_;
                }
            }
        }
    }

    if (stable_ && !holdFired_ && static_cast<Millis>(now - pressedAt_) >= holdMs_) {
        holdFired_ = true;
        events |= Hold;
        pendingClick_ = false;  // click-then-hold is just a hold
        secondPress_ = false;
    }

    if (pendingClick_ && !secondPress_ &&
        static_cast<Millis>(now - pendingSince_) >= doubleClickMs_) {
        events |= Click;  // window passed with no second press
        pendingClick_ = false;
    }

    return events;
}

}  // namespace curl
