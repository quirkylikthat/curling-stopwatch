// Menu: the touch-driven menu opened by a 2 s blue hold (off-ice use).
//
//   Main ── SETTINGS ── BRIGHTNESS (slider)
//     │        └─────── SPLIT BANDS (scrolling list: -/+ per limit, SHIFT ALL; RESET)
//     ├──── EXPORT (coming later)
//     └──── CLEAR HISTORY ── confirm
//
// Pure logic: it hit-tests touches against ui/MenuLayout and edits a
// UserSettings in place; the App applies/persists the result. Unit-tested.
//
// Touch input (see Hardware::pollTouch): down -> drags -> (tap if the finger
// barely moved). The slider follows down/drag directly; the band list scrolls
// once a drag exceeds config::kTouchTapSlopPx, so taps never scroll it.
#pragma once

#include <stdint.h>

#include "../ui/ViewModel.h"
#include "UserSettings.h"

namespace curl {

class Menu {
public:
    using Page = MenuPage;  // defined with the view (ui/ViewModel.h)

    enum class Action : uint8_t {
        None,               // nothing was hit
        Navigated,          // moved between pages
        SettingsChanged,    // split limits changed
        BrightnessChanged,  // apply UserSettings::brightness now (live preview)
        Scrolled,           // the band list moved (redraw only)
        ClearHistory,       // the user confirmed
        Exit,               // leave the menu (back from Main)
    };

    void open() {
        page_ = Page::Main;
        scroll_ = 0;
    }
    Page page() const { return page_; }
    int bandsScroll() const { return scroll_; }  // px the band list is scrolled down

    Action down(int x, int y, UserSettings& s);  // finger landed
    Action drag(int x, int y, UserSettings& s);  // finger moved
    Action tap(int x, int y, UserSettings& s);   // finger lifted near its landing point
    Action back();                               // blue click, or a BACK button

private:
    Action go(Page p) {
        page_ = p;
        if (p == Page::Bands) scroll_ = 0;
        return Action::Navigated;
    }
    Action tapBands(int x, int y, UserSettings& s);

    Page page_ = Page::Main;
    int scroll_ = 0;

    // Current gesture on the band list.
    bool listGesture_ = false;  // landed inside the scrolling list
    bool scrolling_ = false;    // moved past the tap slop: now scrolling
    int anchorY_ = 0;
    int anchorScroll_ = 0;
};

}  // namespace curl
