// Layout: vertical placement of the lines between the big number and the
// history row on the timing screen. Pure (no drawing), unit-tested.
//
// Whatever is shown -- the band label ("DRAW"), the secondary line
// ("SPLIT 3.82 DRAW"), both, or neither -- is spaced so the gaps between the
// big number's ink, each line's ink, and the history row's ink are equal.
#pragma once

#include "Theme.h"

namespace curl {
namespace layout {

struct MidLines {
    int bandBaseline;       // draw with baseline_center; 0 = not shown
    int secondaryBaseline;  // draw with baseline_center; 0 = not shown
    int gap;                // the equal gap in px (for tests / tuning)
};

constexpr MidLines midLines(bool showBand, bool showSecondary) {
    using namespace theme;
    const int bandInk = showBand ? kBandInkAbove + kBandInkBelow : 0;
    const int secInk = showSecondary ? kSecondaryInkAbove + kSecondaryInkBelow : 0;
    const int lines = (showBand ? 1 : 0) + (showSecondary ? 1 : 0);
    const int space = kHistoryInkTop - kMainInkBottom - bandInk - secInk;
    const int gap = space / (lines + 1);

    MidLines out{0, 0, gap};
    int y = kMainInkBottom + gap;  // top of the next line's ink
    if (showBand) {
        out.bandBaseline = y + kBandInkAbove;
        y += bandInk + gap;
    }
    if (showSecondary) {
        out.secondaryBaseline = y + kSecondaryInkAbove;
    }
    return out;
}

// Curved text: where each character of a string sits on a circle.
// advance[i] = pixel width of character i, radius = circle radius through the
// middle of the letters, tracking = extra px between letters. Writes each
// character's centre angle in degrees, measured from the middle of the string
// (negative = left of it), so the caller can rotate each letter into place.
constexpr float kDegreesPerRadian = 57.29578f;

inline void arcAngles(const int* advance, int count, int radius, int tracking, float* outDeg) {
    int total = 0;
    for (int i = 0; i < count; ++i) total += advance[i] + (i ? tracking : 0);
    int run = 0;
    for (int i = 0; i < count; ++i) {
        const float middle = static_cast<float>(run) + advance[i] / 2.0f - total / 2.0f;
        outDeg[i] = middle / static_cast<float>(radius) * kDegreesPerRadian;
        run += advance[i] + tracking;
    }
}

}  // namespace layout
}  // namespace curl
