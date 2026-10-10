// MenuLayout: where every touch target on the menu pages sits. Shared by the
// Renderer (draws them) and app/Menu (hit-tests taps), so the two can never
// disagree. Pure data; positions checked against the 466 px round screen by
// test/test_settings_menu.cpp.
//
// Each target has a drawn rectangle and a (larger) hit zone: fingers land a
// little off, especially near the round edge, so taps are accepted with some
// padding -- and BACK accepts anything from just above it to the screen edge.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../Config.h"
#include "Theme.h"

namespace curl {
namespace menu_layout {

using theme::kCenter;
using theme::kSize;

struct Rect {
    int16_t cx, cy, w, h;  // centre + size
    constexpr bool contains(int x, int y, int pad = 0) const {
        return x >= cx - w / 2 - pad && x <= cx + w / 2 + pad && y >= cy - h / 2 - pad &&
               y <= cy + h / 2 + pad;
    }
    constexpr int left() const { return cx - w / 2; }
    constexpr int top() const { return cy - h / 2; }
};

constexpr int kTitleY = 70;
constexpr int kItemPad = 7;  // hit padding for stacked menu items (they're 14 px apart)

// Main menu: SETTINGS / EXPORT / CLEAR HISTORY, then BACK.
constexpr Rect kMainItems[] = {
    {kCenter, 152, 280, 56},
    {kCenter, 222, 280, 56},
    {kCenter, 292, 280, 56},
};

// BACK (every page except Bands): drawn here ...
constexpr Rect kBack = {kCenter, 378, 150, 50};
// ... accepted anywhere from just above it down to the bottom edge.
constexpr Rect kBackHit = {kCenter, 400, 210, 132};  // y 334..466

// Settings: BRIGHTNESS / SPLIT BANDS, then BACK.
constexpr Rect kSettingsItems[] = {
    {kCenter, 180, 280, 56},
    {kCenter, 252, 280, 56},
};

// Brightness: a horizontal slider. Touches anywhere in kSliderHit move it.
constexpr int kSliderY = 250;
constexpr int kSliderX0 = kCenter - 150;
constexpr int kSliderX1 = kCenter + 150;
constexpr int kSliderKnobR = 16;
constexpr Rect kSliderHit = {kCenter, kSliderY, 340, 90};
// Slider position <-> brightness (kBrightnessMin..255).
constexpr int sliderX(uint8_t brightness) {
    return kSliderX0 + (brightness - config::kBrightnessMin) * (kSliderX1 - kSliderX0) /
                           (255 - config::kBrightnessMin);
}
constexpr int brightnessAt(int x) {
    return x <= kSliderX0 ? config::kBrightnessMin
           : x >= kSliderX1
               ? 255
               : config::kBrightnessMin + ((x - kSliderX0) * (255 - config::kBrightnessMin) +
                                           (kSliderX1 - kSliderX0) / 2) /
                                              (kSliderX1 - kSliderX0);
}
constexpr int kBrightnessValueY = 170;

// Split bands: fixed title + column header at the top, fixed RESET / BACK at
// the bottom, and a vertically scrolling list in between -- one row per limit
// (label, value, -, +) plus a SHIFT ALL row. Drag the list up/down to scroll.
constexpr int kBandsTitleY = 50;
constexpr int kBandHeaderY = 78;
constexpr int kBandViewTop = 96;      // scrolling viewport (screen y) ...
constexpr int kBandViewBottom = 342;  // ... rows are clipped to it
constexpr int kBandRowPitch = 62;
constexpr int kBandBtnW = 64;
constexpr int kBandBtnH = 50;
constexpr int kBandBtnPad = 4;              // the -/+ pair is 8 px apart
constexpr int kBandLabelX = kCenter - 170;  // left edge
constexpr int kBandValueX = kCenter + 5;    // centre
constexpr int kBandMinusX = kCenter + 70;
constexpr int kBandPlusX = kCenter + 142;
constexpr int kBandScrollbarX = kCenter + 182;
// Rows: one per band limit (bands - 1) + SHIFT ALL = number of bands.
constexpr size_t kBandRows = sizeof(config::kSplitBands) / sizeof(config::kSplitBands[0]);
// Blank space after the last row, so when scrolled to the bottom SHIFT ALL
// sits well clear of the RESET / BACK footer (it was too easy to hit BACK).
constexpr int kBandListEndPad = 44;
constexpr int kBandContentH = static_cast<int>(kBandRows) * kBandRowPitch + kBandListEndPad;
constexpr int kBandViewH = kBandViewBottom - kBandViewTop;
constexpr int kBandMaxScroll = kBandContentH > kBandViewH ? kBandContentH - kBandViewH : 0;

constexpr int bandRowY(size_t row, int scroll) {  // row centre, screen y
    return kBandViewTop + static_cast<int>(row) * kBandRowPitch + kBandRowPitch / 2 - scroll;
}
constexpr Rect bandMinus(size_t row, int scroll) {
    return {kBandMinusX, static_cast<int16_t>(bandRowY(row, scroll)), kBandBtnW, kBandBtnH};
}
constexpr Rect bandPlus(size_t row, int scroll) {
    return {kBandPlusX, static_cast<int16_t>(bandRowY(row, scroll)), kBandBtnW, kBandBtnH};
}
constexpr bool inBandView(int y) { return y >= kBandViewTop && y < kBandViewBottom; }

constexpr Rect kBandsReset = {kCenter - 75, 388, 130, 50};
constexpr Rect kBandsBack = {kCenter + 75, 388, 130, 50};
// Footer hit zones: each half of the screen below the list, to the edge,
// starting kBandFooterGap below the list so the two never touch.
constexpr int kBandFooterGap = 14;
constexpr Rect kBandsResetHit = {kCenter - 76, 412, 150, 108};  // y 358..466
constexpr Rect kBandsBackHit = {kCenter + 76, 412, 150, 108};

// Confirm "clear history".
constexpr int kConfirmTextY = 190;
constexpr Rect kConfirmCancel = {kCenter - 75, 300, 130, 54};
constexpr Rect kConfirmClear = {kCenter + 75, 300, 130, 54};
constexpr int kConfirmPad = 8;

}  // namespace menu_layout
}  // namespace curl
