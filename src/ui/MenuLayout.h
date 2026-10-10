// MenuLayout: where every touch target on the menu pages sits. Shared by the
// Renderer (draws them) and app/Menu (hit-tests taps), so the two can never
// disagree. Pure data; positions checked against the 466 px round screen.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "Theme.h"

namespace curl {
namespace menu_layout {

using theme::kCenter;

struct Rect {
    int16_t cx, cy, w, h;  // centre + size
    constexpr bool contains(int x, int y) const {
        return x >= cx - w / 2 && x <= cx + w / 2 && y >= cy - h / 2 && y <= cy + h / 2;
    }
    constexpr int left() const { return cx - w / 2; }
    constexpr int top() const { return cy - h / 2; }
};

constexpr int kTitleY = 70;

// Main menu: SETTINGS / EXPORT / CLEAR HISTORY, then BACK.
constexpr Rect kMainItems[] = {
    {kCenter, 152, 280, 56},
    {kCenter, 222, 280, 56},
    {kCenter, 292, 280, 56},
};
constexpr Rect kBack = {kCenter, 378, 150, 50};  // every page except Bands

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

// Split bands: one row per limit (label, value, -, +), then SHIFT ALL, then
// RESET + BACK side by side. Rows are 42 px apart; buttons 52 x 36.
constexpr int kBandRowY0 = 108;
constexpr int kBandRowStep = 42;
constexpr int kBandLabelX = kCenter - 168;  // left edge
constexpr int kBandValueX = kCenter + 22;   // centre
constexpr int kBandMinusX = kCenter + 92;
constexpr int kBandPlusX = kCenter + 152;
constexpr int kBandBtnW = 52;
constexpr int kBandBtnH = 36;
constexpr int kBandHeaderY = 84;
constexpr int kBandsTitleY = 54;  // higher than kTitleY: the rows need the room
constexpr int bandRowY(size_t row) { return kBandRowY0 + static_cast<int>(row) * kBandRowStep; }
constexpr Rect bandMinus(size_t row) {
    return {kBandMinusX, static_cast<int16_t>(bandRowY(row)), kBandBtnW, kBandBtnH};
}
constexpr Rect bandPlus(size_t row) {
    return {kBandPlusX, static_cast<int16_t>(bandRowY(row)), kBandBtnW, kBandBtnH};
}
constexpr Rect kBandsReset = {kCenter - 70, 378, 120, 46};
constexpr Rect kBandsBack = {kCenter + 70, 378, 120, 46};

// Confirm "clear history".
constexpr int kConfirmTextY = 190;
constexpr Rect kConfirmCancel = {kCenter - 75, 300, 130, 54};
constexpr Rect kConfirmClear = {kCenter + 75, 300, 130, 54};

}  // namespace menu_layout
}  // namespace curl
