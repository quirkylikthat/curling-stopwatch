#include "Menu.h"

#include "../ui/MenuLayout.h"

namespace curl {

using namespace menu_layout;

static_assert(kBandRows == kSplitLimitCount + 1, "band list = one row per limit + SHIFT ALL");

namespace {
int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
}  // namespace

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

Menu::Action Menu::down(int x, int y, UserSettings& s) {
    listGesture_ = false;
    scrolling_ = false;
    if (page_ == Page::Bands && inBandView(y)) {
        listGesture_ = true;
        anchorY_ = y;
        anchorScroll_ = scroll_;
        return Action::None;
    }
    return drag(x, y, s);  // the slider jumps to where the finger lands
}

Menu::Action Menu::drag(int x, int y, UserSettings& s) {
    if (page_ == Page::Brightness && kSliderHit.contains(x, y)) {
        return settings::setBrightness(s, brightnessAt(x)) ? Action::BrightnessChanged
                                                           : Action::None;
    }
    if (page_ == Page::Bands && listGesture_) {
        const int dy = y - anchorY_;
        if (!scrolling_ && (dy > config::kTouchTapSlopPx || -dy > config::kTouchTapSlopPx)) {
            scrolling_ = true;
        }
        if (scrolling_) {
            const int next =
                clampInt(anchorScroll_ - dy, 0, kBandMaxScroll);  // drag up = scroll down
            if (next != scroll_) {
                scroll_ = next;
                return Action::Scrolled;
            }
        }
    }
    return Action::None;
}

Menu::Action Menu::tap(int x, int y, UserSettings& s) {
    switch (page_) {
        case Page::Main:
            if (kMainItems[0].contains(x, y, kItemPad)) return go(Page::Settings);
            if (kMainItems[1].contains(x, y, kItemPad)) return go(Page::Export);
            if (kMainItems[2].contains(x, y, kItemPad)) return go(Page::ConfirmClear);
            if (kBackHit.contains(x, y)) return back();
            return Action::None;

        case Page::Settings:
            if (kSettingsItems[0].contains(x, y, kItemPad)) return go(Page::Brightness);
            if (kSettingsItems[1].contains(x, y, kItemPad)) return go(Page::Bands);
            if (kBackHit.contains(x, y)) return back();
            return Action::None;

        case Page::Brightness:
            if (kSliderHit.contains(x, y)) return Action::None;  // down/drag already moved it
            if (kBackHit.contains(x, y)) return back();
            return Action::None;

        case Page::Bands:
            return tapBands(x, y, s);

        case Page::Export:
            if (kBackHit.contains(x, y)) return back();
            return Action::None;

        case Page::ConfirmClear:
            if (kConfirmClear.contains(x, y, kConfirmPad)) {
                page_ = Page::Main;
                return Action::ClearHistory;
            }
            if (kConfirmCancel.contains(x, y, kConfirmPad)) return go(Page::Main);
            return Action::None;
    }
    return Action::None;
}

Menu::Action Menu::tapBands(int x, int y, UserSettings& s) {
    if (inBandView(y)) {
        const int step = settings::kLimitStepCs;
        for (size_t row = 0; row < kBandRows; ++row) {
            const bool isShift = row == kSplitLimitCount;  // last row: SHIFT ALL
            int delta = 0;
            if (bandMinus(row, scroll_).contains(x, y, kBandBtnPad)) delta = -step;
            if (bandPlus(row, scroll_).contains(x, y, kBandBtnPad)) delta = +step;
            if (delta == 0) continue;
            const bool changed =
                isShift ? settings::shiftAllLimits(s, delta) : settings::adjustLimit(s, row, delta);
            return changed ? Action::SettingsChanged : Action::None;
        }
        return Action::None;  // the list, but not a button
    }
    if (kBandsResetHit.contains(x, y)) {
        return settings::resetSplitLimits(s) ? Action::SettingsChanged : Action::None;
    }
    if (kBandsBackHit.contains(x, y)) return back();
    return Action::None;
}

}  // namespace curl
