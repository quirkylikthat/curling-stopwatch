// ButtonLogic: debounce + click/hold detection for one physical button.
//
// Feed it the raw pressed/not-pressed level every loop; it returns a bitmask
// of events that happened on this update. Pure logic, no GPIO access, so it is
// unit-tested on the PC. Used for the blue (control) button. The yellow
// (timing) button bypasses this and is timestamped in an interrupt instead,
// because debouncing by polling would add up to one loop period of jitter.
#pragma once

#include "Types.h"

namespace curl {

class ButtonLogic {
public:
    enum Event : uint8_t {
        None = 0,
        Pressed = 1 << 0,   // debounced press
        Released = 1 << 1,  // debounced release
        Click = 1 << 2,     // released before the hold threshold
        Hold = 1 << 3,      // held past the hold threshold (fires once, while still down)
    };

    ButtonLogic(Millis debounceMs, Millis holdMs) : debounceMs_(debounceMs), holdMs_(holdMs) {}

    uint8_t update(Millis now, bool rawPressed);
    bool isPressed() const { return stable_; }

    // Swallow the current press: no events until the button has been released.
    // Used when a press only woke the device from sleep (or was held at boot),
    // so it must not also act as a click.
    void suppressUntilRelease() { suppress_ = true; }

private:
    bool suppress_ = false;
    Millis debounceMs_;
    Millis holdMs_;

    bool stable_ = false;
    bool lastRaw_ = false;
    Millis rawChangedAt_ = 0;
    Millis pressedAt_ = 0;
    bool holdFired_ = false;
};

}  // namespace curl
