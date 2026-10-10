// Menu: the touch-driven menu opened by a 2 s blue hold (off-ice use).
//
//   Main ── SETTINGS ── BRIGHTNESS (slider)
//     │        └─────── SPLIT BANDS (-/+ per limit, SHIFT ALL, RESET)
//     ├──── EXPORT (coming later)
//     └──── CLEAR HISTORY ── confirm
//
// Pure logic: it hit-tests touches against ui/MenuLayout and edits a
// UserSettings in place; the App applies/persists the result. Unit-tested.
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
        ClearHistory,       // the user confirmed
        Exit,               // leave the menu (back from Main)
    };

    void open() { page_ = Page::Main; }
    Page page() const { return page_; }

    Action tap(int x, int y, UserSettings& s);   // finger lifted (a click)
    Action drag(int x, int y, UserSettings& s);  // finger down / moving
    Action back();                               // blue click, or a BACK button

private:
    Action go(Page p) {
        page_ = p;
        return Action::Navigated;
    }

    Page page_ = Page::Main;
};

}  // namespace curl
