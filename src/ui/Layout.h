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

}  // namespace layout
}  // namespace curl
