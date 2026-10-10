// UserSettings editing rules, the touch Menu, and the menu layout on the round screen.
#include "../src/app/Menu.h"
#include "../src/app/UserSettings.h"
#include "../src/ui/MenuLayout.h"
#include "TestFramework.h"

using namespace curl;
using namespace curl::menu_layout;

namespace {
UserSettings def() { return settings::defaults(); }
bool tapRect(Menu& m, const Rect& r, UserSettings& s, Menu::Action expect) {
    return m.tap(r.cx, r.cy, s) == expect;
}
}  // namespace

// ---- UserSettings -----------------------------------------------------------

TEST(settings_defaults_match_config_and_are_valid) {
    const UserSettings s = def();
    CHECK(settings::isValid(s));
    CHECK_EQ(s.brightness, config::kBrightnessActive);
    CHECK_EQ(kSplitLimitCount, 5u);
    CHECK_EQ(s.splitLimitsCs[0], 290);  // PEEL < 2.90 (float 2.90f must not truncate to 289)
    CHECK_EQ(s.splitLimitsCs[4], 415);  // DRAW < 4.15
}

TEST(settings_adjust_limit_keeps_order) {
    UserSettings s = def();  // 290 320 350 375 415
    CHECK(settings::adjustLimit(s, 1, +5));
    CHECK_EQ(s.splitLimitsCs[1], 325);
    // Push TAKEOUT up into BOARD: stops 0.05 s below it.
    for (int i = 0; i < 20; ++i) settings::adjustLimit(s, 1, +5);
    CHECK_EQ(s.splitLimitsCs[1], 345);
    CHECK(!settings::adjustLimit(s, 1, +5));  // no change -> false
    CHECK(settings::isValid(s));
    // Lowest limit can't go below 1.50 s.
    for (int i = 0; i < 100; ++i) settings::adjustLimit(s, 0, -5);
    CHECK_EQ(s.splitLimitsCs[0], settings::kLimitMinCs);
    CHECK(!settings::adjustLimit(s, 9, +5));  // out of range index
}

TEST(settings_shift_all_moves_together_and_clamps) {
    UserSettings s = def();
    CHECK(settings::shiftAllLimits(s, +10));
    CHECK_EQ(s.splitLimitsCs[0], 300);
    CHECK_EQ(s.splitLimitsCs[4], 425);
    // Way past the top: the whole set stops when DRAW hits 8.00 s, spacing intact.
    settings::shiftAllLimits(s, +10000);
    CHECK_EQ(s.splitLimitsCs[4], settings::kLimitMaxCs);
    CHECK_EQ(s.splitLimitsCs[4] - s.splitLimitsCs[0], 415 - 290);
    CHECK(!settings::shiftAllLimits(s, +5));
    settings::shiftAllLimits(s, -10000);
    CHECK_EQ(s.splitLimitsCs[0], settings::kLimitMinCs);
    CHECK(settings::isValid(s));
}

TEST(settings_reset_and_validation) {
    UserSettings s = def();
    settings::shiftAllLimits(s, +50);
    CHECK(settings::resetSplitLimits(s));
    CHECK_EQ(s.splitLimitsCs[2], 350);
    CHECK(!settings::resetSplitLimits(s));

    UserSettings bad = def();
    bad.splitLimitsCs[3] = bad.splitLimitsCs[2];  // not ascending
    CHECK(!settings::isValid(bad));
    bad = def();
    bad.brightness = 5;
    CHECK(!settings::isValid(bad));
    UserSettings zero{};  // what an empty / corrupt NVS read looks like
    CHECK(!settings::isValid(zero));
}

TEST(settings_brightness_clamped) {
    UserSettings s = def();
    CHECK(settings::setBrightness(s, 0));
    CHECK_EQ(s.brightness, settings::kBrightnessMin);
    settings::setBrightness(s, 999);
    CHECK_EQ(s.brightness, 255);
    CHECK_EQ(settings::brightnessPercent(255), 100);
    CHECK_EQ(settings::brightnessPercent(0), 0);
}

TEST(settings_split_bands_follow_user_limits) {
    UserSettings s = def();
    settings::shiftAllLimits(s, +20);  // keen ice: every band 0.20 s later
    const settings::SplitBands b = settings::makeSplitBands(s);
    CHECK_STR(classify(b.bands, 3.0f)->label, "PEEL");  // was TAKEOUT with defaults
    CHECK_STR(classify(b.bands, 4.30f)->label, "DRAW");
    CHECK_STR(classify(b.bands, 9.0f)->label, "GUARD");
    CHECK_EQ(b.bands[1].color, config::kSplitBands[1].color);
}

// ---- Menu -------------------------------------------------------------------

TEST(menu_navigation_and_back) {
    Menu m;
    UserSettings s = def();
    m.open();
    CHECK(m.page() == Menu::Page::Main);
    CHECK(tapRect(m, kMainItems[0], s, Menu::Action::Navigated));
    CHECK(m.page() == Menu::Page::Settings);
    CHECK(tapRect(m, kSettingsItems[1], s, Menu::Action::Navigated));
    CHECK(m.page() == Menu::Page::Bands);
    CHECK(m.back() == Menu::Action::Navigated);  // blue click
    CHECK(m.page() == Menu::Page::Settings);
    CHECK(tapRect(m, kBack, s, Menu::Action::Navigated));
    CHECK(m.page() == Menu::Page::Main);
    CHECK(m.back() == Menu::Action::Exit);
    CHECK(tapRect(m, kMainItems[1], s, Menu::Action::Navigated));
    CHECK(m.page() == Menu::Page::Export);
    CHECK(m.tap(5, 5, s) == Menu::Action::None);  // nothing there
}

TEST(menu_clear_history_needs_confirmation) {
    Menu m;
    UserSettings s = def();
    m.open();
    CHECK(tapRect(m, kMainItems[2], s, Menu::Action::Navigated));
    CHECK(m.page() == Menu::Page::ConfirmClear);
    CHECK(tapRect(m, kConfirmCancel, s, Menu::Action::Navigated));
    CHECK(m.page() == Menu::Page::Main);
    tapRect(m, kMainItems[2], s, Menu::Action::Navigated);
    CHECK(tapRect(m, kConfirmClear, s, Menu::Action::ClearHistory));
    CHECK(m.page() == Menu::Page::Main);
}

TEST(menu_brightness_slider) {
    Menu m;
    UserSettings s = def();
    m.open();
    tapRect(m, kMainItems[0], s, Menu::Action::Navigated);
    tapRect(m, kSettingsItems[0], s, Menu::Action::Navigated);
    CHECK(m.page() == Menu::Page::Brightness);
    CHECK(m.drag(kSliderX1 + 20, kSliderY, s) == Menu::Action::BrightnessChanged);
    CHECK_EQ(s.brightness, 255);
    CHECK(m.drag(kSliderX0 - 20, kSliderY + 30, s) == Menu::Action::BrightnessChanged);
    CHECK_EQ(s.brightness, settings::kBrightnessMin);
    CHECK(m.drag(kSliderX0 - 20, kSliderY, s) == Menu::Action::None);  // unchanged
    CHECK(m.drag(kCenter, 60, s) == Menu::Action::None);               // off the slider
    m.tap((kSliderX0 + kSliderX1) / 2, kSliderY, s);
    CHECK(s.brightness > 130 && s.brightness < 145);  // ~midway
    // Knob position round-trips.
    CHECK(brightnessAt(sliderX(200)) - 200 <= 1);
}

TEST(menu_band_buttons_edit_the_right_limit) {
    Menu m;
    UserSettings s = def();
    m.open();
    tapRect(m, kMainItems[0], s, Menu::Action::Navigated);
    tapRect(m, kSettingsItems[1], s, Menu::Action::Navigated);
    CHECK(tapRect(m, bandPlus(4), s, Menu::Action::SettingsChanged));  // DRAW
    CHECK_EQ(s.splitLimitsCs[4], 420);
    CHECK(tapRect(m, bandMinus(0), s, Menu::Action::SettingsChanged));  // PEEL
    CHECK_EQ(s.splitLimitsCs[0], 285);
    CHECK(tapRect(m, bandPlus(kSplitLimitCount), s, Menu::Action::SettingsChanged));  // shift
    CHECK_EQ(s.splitLimitsCs[0], 290);
    CHECK_EQ(s.splitLimitsCs[4], 425);
    CHECK(tapRect(m, kBandsReset, s, Menu::Action::SettingsChanged));
    CHECK_EQ(s.splitLimitsCs[4], 415);
    CHECK(tapRect(m, kBandsReset, s, Menu::Action::None));  // already default
    CHECK(tapRect(m, kBandsBack, s, Menu::Action::Navigated));
    CHECK(m.page() == Menu::Page::Settings);
}

// ---- Layout -----------------------------------------------------------------

namespace {
bool insideCircle(const Rect& r) {
    const int R = 230;  // visible radius is 233; keep a small margin
    const int xs[2] = {r.left(), r.left() + r.w};
    const int ys[2] = {r.top(), r.top() + r.h};
    for (int x : xs) {
        for (int y : ys) {
            const int dx = x - kCenter, dy = y - kCenter;
            if (dx * dx + dy * dy > R * R) return false;
        }
    }
    return true;
}
bool overlap(const Rect& a, const Rect& b) {
    return a.left() < b.left() + b.w && b.left() < a.left() + a.w && a.top() < b.top() + b.h &&
           b.top() < a.top() + a.h;
}
}  // namespace

TEST(menu_layout_targets_fit_on_round_screen_without_overlap) {
    for (const Rect& r : kMainItems) CHECK(insideCircle(r));
    for (const Rect& r : kSettingsItems) CHECK(insideCircle(r));
    CHECK(insideCircle(kBack));
    CHECK(insideCircle(kSliderHit));
    CHECK(insideCircle(kConfirmCancel));
    CHECK(insideCircle(kConfirmClear));
    CHECK(insideCircle(kBandsReset));
    CHECK(insideCircle(kBandsBack));
    for (size_t row = 0; row <= kSplitLimitCount; ++row) {  // limits + SHIFT ALL
        CHECK(insideCircle(bandMinus(row)));
        CHECK(insideCircle(bandPlus(row)));
        CHECK(!overlap(bandMinus(row), bandPlus(row)));
        if (row > 0) CHECK(!overlap(bandPlus(row - 1), bandPlus(row)));
    }
    CHECK(!overlap(bandPlus(kSplitLimitCount), kBandsBack));
    CHECK(!overlap(kMainItems[2], kBack));
}
