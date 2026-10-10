#include "Menu.h"

#include "../ui/MenuLayout.h"

namespace curl {

using namespace menu_layout;

Menu::Action Menu::back() {
    switch (page_) {
        case Page::Main:
            return Action::Exit;
        case Page::Brightness:
        case Page::Bands:
            return go(Page::Settings);
        case Page::Settings:
        case Page::Export:
        case Page::ConfirmClear:
            return go(Page::Main);
    }
    return Action::None;
}

Menu::Action Menu::drag(int x, int y, UserSettings& s) {
    if (page_ == Page::Brightness && kSliderHit.contains(x, y)) {
        return settings::setBrightness(s, brightnessAt(x)) ? Action::BrightnessChanged
                                                           : Action::None;
    }
    return Action::None;
}

Menu::Action Menu::tap(int x, int y, UserSettings& s) {
    switch (page_) {
        case Page::Main:
            if (kMainItems[0].contains(x, y)) return go(Page::Settings);
            if (kMainItems[1].contains(x, y)) return go(Page::Export);
            if (kMainItems[2].contains(x, y)) return go(Page::ConfirmClear);
            if (kBack.contains(x, y)) return back();
            return Action::None;

        case Page::Settings:
            if (kSettingsItems[0].contains(x, y)) return go(Page::Brightness);
            if (kSettingsItems[1].contains(x, y)) return go(Page::Bands);
            if (kBack.contains(x, y)) return back();
            return Action::None;

        case Page::Brightness:
            if (kSliderHit.contains(x, y)) return drag(x, y, s);  // tapping the track jumps there
            if (kBack.contains(x, y)) return back();
            return Action::None;

        case Page::Bands: {
            const int step = settings::kLimitStepCs;
            for (size_t row = 0; row < kSplitLimitCount; ++row) {
                if (bandMinus(row).contains(x, y)) {
                    return settings::adjustLimit(s, row, -step) ? Action::SettingsChanged
                                                                : Action::None;
                }
                if (bandPlus(row).contains(x, y)) {
                    return settings::adjustLimit(s, row, +step) ? Action::SettingsChanged
                                                                : Action::None;
                }
            }
            const size_t shiftRow = kSplitLimitCount;  // the row after the limits
            if (bandMinus(shiftRow).contains(x, y)) {
                return settings::shiftAllLimits(s, -step) ? Action::SettingsChanged : Action::None;
            }
            if (bandPlus(shiftRow).contains(x, y)) {
                return settings::shiftAllLimits(s, +step) ? Action::SettingsChanged : Action::None;
            }
            if (kBandsReset.contains(x, y)) {
                return settings::resetSplitLimits(s) ? Action::SettingsChanged : Action::None;
            }
            if (kBandsBack.contains(x, y)) return back();
            return Action::None;
        }

        case Page::Export:
            if (kBack.contains(x, y)) return back();
            return Action::None;

        case Page::ConfirmClear:
            if (kConfirmClear.contains(x, y)) {
                page_ = Page::Main;
                return Action::ClearHistory;
            }
            if (kConfirmCancel.contains(x, y)) return go(Page::Main);
            return Action::None;
    }
    return Action::None;
}

}  // namespace curl
