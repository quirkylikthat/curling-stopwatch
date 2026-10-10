// ButtonLogic: debounce + click / double-click / hold detection for one button.
//
// Feed it the raw pressed/not-pressed level every loop; it returns a bitmask
// of events that happened on this update. Pure logic, no GPIO access, so it is
// unit-tested on the PC. Used for the blue (control) button. The yellow
// (timing) button bypasses this and is timestamped in an interrupt instead,
// because debouncing by polling would add up to one loop period of jitter.
//
// Double-click (optional, doubleClickMs > 0): a single Click is then reported
// only once the window has passed without a second press, i.e. ~doubleClickMs
// after release. Click-then-hold reports only Hold.
#pragma once

#include "Types.h"

namespace curl {

class ButtonLogic {
public:
    enum Event : uint8_t {
        None = 0,
        Pressed = 1 << 0,      // debounced press
        Released = 1 << 1,     // debounced release
        Click = 1 << 2,        // single short press (delayed if double-click is enabled)
        Hold = 1 << 3,         // held past the hold threshold (fires once, while still down)
        DoubleClick = 1 << 4,  // two short presses within the double-click window
    };

    // doubleClickMs = 0 disables double-click: Click fires immediately on release.
    ButtonLogic(Millis debounceMs, Millis holdMs, Millis doubleClickMs = 0)
        : debounceMs_(debounceMs), holdMs_(holdMs), doubleClickMs_(doubleClickMs) {}

    uint8_t update(Millis now, bool rawPressed);
    bool isPressed() const { return stable_; }

    // Swallow the current press: no events until the button has been released.
    // Used when a press only woke the device from sleep (or was held at boot),
    // so it must not also act as a click.
    void suppressUntilRelease() {
        suppress_ = true;
        pendingClick_ = false;
        secondPress_ = false;
    }

private:
    bool suppress_ = false;
    Millis debounceMs_;
    Millis holdMs_;
    Millis doubleClickMs_;

    bool stable_ = false;
    bool lastRaw_ = false;
    Millis rawChangedAt_ = 0;
    Millis pressedAt_ = 0;
    bool holdFired_ = false;

    bool pendingClick_ = false;  // first click seen; waiting to see if a second follows
    Millis pendingSince_ = 0;    // release time of that first click
    bool secondPress_ = false;   // second press of a possible double-click is in progress
};

}  // namespace curl
