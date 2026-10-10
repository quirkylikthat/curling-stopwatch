// Timing-screen vertical layout: the band label / secondary line must sit with
// equal gaps between the big number and the history row (Mike's bug report:
// "DRAW" was touching the big number).
#include "../src/ui/Layout.h"
#include "TestFramework.h"

using namespace curl;
using namespace curl::theme;

namespace {
int abs_(int v) { return v < 0 ? -v : v; }
}  // namespace

TEST(layout_ink_edges_match_font_measurements) {
    // From scripts/fontmetrics.py with the default positions (kMainY 222, kHistoryY 392).
    CHECK_EQ(kMainInkBottom, 275);
    CHECK_EQ(kHistoryInkTop, 382);
}

TEST(layout_band_alone_is_centred_with_equal_gaps) {
    const auto m = layout::midLines(true, false);
    const int gapAbove = (m.bandBaseline - kBandInkAbove) - kMainInkBottom;
    const int gapBelow = kHistoryInkTop - (m.bandBaseline + kBandInkBelow);
    CHECK(gapAbove >= 30);  // was ~0 before the fix
    CHECK(abs_(gapAbove - gapBelow) <= 1);
    CHECK_EQ(m.secondaryBaseline, 0);
}

TEST(layout_secondary_alone_is_centred_with_equal_gaps) {
    const auto m = layout::midLines(false, true);
    const int gapAbove = (m.secondaryBaseline - kSecondaryInkAbove) - kMainInkBottom;
    const int gapBelow = kHistoryInkTop - (m.secondaryBaseline + kSecondaryInkBelow);
    CHECK(abs_(gapAbove - gapBelow) <= 1);
    CHECK_EQ(m.bandBaseline, 0);
}

TEST(layout_band_and_secondary_three_equal_gaps) {
    const auto m = layout::midLines(true, true);
    const int g1 = (m.bandBaseline - kBandInkAbove) - kMainInkBottom;
    const int g2 = (m.secondaryBaseline - kSecondaryInkAbove) - (m.bandBaseline + kBandInkBelow);
    const int g3 = kHistoryInkTop - (m.secondaryBaseline + kSecondaryInkBelow);
    CHECK(g1 > 0 && g2 > 0 && g3 > 0);
    CHECK(abs_(g1 - g2) <= 2);
    CHECK(abs_(g2 - g3) <= 2);
}

TEST(layout_nothing_shown) {
    const auto m = layout::midLines(false, false);
    CHECK_EQ(m.bandBaseline, 0);
    CHECK_EQ(m.secondaryBaseline, 0);
}
