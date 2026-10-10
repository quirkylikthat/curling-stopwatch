// Theme: colours and layout constants for the 466x466 round AMOLED.
//
// AMOLED notes: black pixels are OFF (free on battery, infinite contrast), and
// static bright elements can burn in over time. So: black background, keep
// big static shapes dim, and dim the whole screen when idle.
#pragma once

#include <stdint.h>

#include "../Config.h"

namespace curl {
namespace theme {

using config::rgb565;

constexpr uint16_t kBackground = 0x0000;
constexpr uint16_t kText = rgb565(240, 240, 240);
constexpr uint16_t kTextDim = rgb565(120, 120, 120);
constexpr uint16_t kTextFaint = rgb565(70, 70, 70);
constexpr uint16_t kRunning = rgb565(255, 200, 40);  // live value colour
constexpr uint16_t kAccent = rgb565(80, 190, 255);
constexpr uint16_t kWarn = rgb565(255, 90, 60);
constexpr uint16_t kBattOk = rgb565(60, 200, 90);
constexpr uint16_t kBattLow = rgb565(255, 70, 50);
constexpr uint16_t kBattCharging = rgb565(80, 170, 255);
constexpr uint16_t kTrack = rgb565(35, 35, 35);
constexpr uint16_t kButton = rgb565(45, 45, 45);         // menu button fill
constexpr uint16_t kButtonDanger = rgb565(120, 25, 20);  // "CLEAR" confirm

// Geometry. The visible area is a circle; anything near the corners is lost.
// Width of the visible chord at height y: 2*sqrt(R^2 - (y-C)^2).
constexpr int kSize = 466;
constexpr int kCenter = kSize / 2;

constexpr int kBatteryR0 = 222;  // battery ring (top arc)
constexpr int kBatteryR1 = 230;
constexpr float kBatteryArcStart = 225.0f;  // degrees, 0 = 3 o'clock, clockwise
constexpr float kBatteryArcSpan = 90.0f;

constexpr int kModeY = 70;      // "SPLIT"
constexpr int kDotsY = 104;     // gate progress dots
constexpr int kHintY = 142;     // "NEXT: NEAR HOG" (or the main readout's caption)
constexpr int kMainY = 222;     // big number (middle_center)
constexpr int kHistoryY = 392;  // previous results (middle_center)
// The band label ("DRAW") and the secondary line ("SPLIT 3.82 DRAW") have no
// fixed Y: ui/Layout.h spaces whichever are shown evenly between the big
// number and the history row.

constexpr float kMainTextSize = 2.3f;  // Font7 (7-segment) is 48 px tall at size 1

// Ink extents (lit pixels, not line boxes) from the font data -- re-measure with
// scripts/fontmetrics.py if a font, size or M5GFX version changes.
//   Font7: 48 px cell, ink rows 1..46. LovyanGFX middle datum puts the cell
//   top at y - h/2. GFX fonts: middle datum top = y - height/2, and the
//   baseline is `baseline` px below that.
constexpr int kMainInkBottom =
    kMainY - static_cast<int>(48 * kMainTextSize) / 2 + static_cast<int>(47 * kMainTextSize);
constexpr int kHistoryInkTop = kHistoryY - 23 / 2 + 17 - 16;  // FreeSans12pt7b digits
constexpr int kBandInkAbove = 34;       // FreeSansBold24pt7b caps: px above baseline ...
constexpr int kBandInkBelow = 2;        // ... and below (round-letter overshoot)
constexpr int kSecondaryInkAbove = 17;  // FreeSansBold12pt7b caps/digits
constexpr int kSecondaryInkBelow = 1;

// History screen (title uses kModeY). Rows run y = 136..379; the visible chord
// is >= 360 px wide there, so three columns fit with margin.
constexpr int kHistHeaderY = 104;
constexpr int kHistFirstRowY = 136;
constexpr int kHistRowStep = 27;
constexpr int kHistNumberX = kCenter - 100;  // right edge of "#12"
constexpr int kHistSplitX = kCenter - 35;    // column centres
constexpr int kHistHogX = kCenter + 75;
constexpr int kHistFooterY = 412;

}  // namespace theme
}  // namespace curl
