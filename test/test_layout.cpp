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

// ---- Boot splash: curling house + curved title --------------------------------

TEST(splash_house_rings_nest_and_fill_the_glass) {
    CHECK_EQ(kHouseR12, kSize / 2);  // the blue ring is the glass edge
    CHECK(kHouseR12 > kHouseR8);
    CHECK(kHouseR8 > kHouseR4);
    CHECK(kHouseR4 > kHouseButtonR);
    CHECK(kHouseButtonR > 0);
    // Real curling proportions: 12 : 8 : 4 : 0.5 feet.
    CHECK(abs_(kHouseR8 * 3 - kHouseR12 * 2) <= 2);
    CHECK(abs_(kHouseR4 * 3 - kHouseR12) <= 2);
}

TEST(splash_curved_text_fits_inside_the_blue_ring) {
    const int half = (kBandInkAbove + kBandInkBelow) / 2;  // half the letter height
    const int halfWidth = 22;                              // widest capital is under 44 px
    CHECK(kSplashTextR - half >= kHouseR8 + 10);           // clear of the white ring
    // The outer corner of a letter stays inside the glass.
    const float corner = std::sqrt(
        static_cast<float>((kSplashTextR + half) * (kSplashTextR + half) + halfWidth * halfWidth));
    CHECK(corner <= kHouseR12 - 8);
}

TEST(splash_version_sits_between_timer_and_the_glass_edge) {
    const int top = kSplashVersionBaseline - kVersionInkAbove;  // ink top, screen y
    const int timerInkBottom = kCenter + kSplashTextR + kSplashHalfInk;
    CHECK(top - timerInkBottom >= 5);                          // clear of "TIMER"
    CHECK(kCenter + kHouseR12 - kSplashVersionBaseline >= 5);  // inside the glass at the centre
    // Its bottom corners (a version string is under 60 px wide) stay inside the round glass.
    const float corner = std::sqrt(static_cast<float>(
        30 * 30 + (kSplashVersionBaseline - kCenter) * (kSplashVersionBaseline - kCenter)));
    CHECK(corner <= kHouseR12 - 4);
}

TEST(splash_arc_angles_are_centred_ordered_and_scale_with_radius) {
    const int adv[] = {30, 26, 34, 30, 22};
    float deg[5];
    layout::arcAngles(adv, 5, 194, 4, deg);
    for (int i = 1; i < 5; ++i) CHECK(deg[i] > deg[i - 1]);
    // Centred on the middle of the string: first and last are mirrored about 0 within
    // the difference in their own half-widths.
    const float expectedFirst =
        -(((30 + 26 + 34 + 30 + 22) + 4 * 4) / 2.0f - 15) / 194.0f * 57.29578f;
    CHECK(std::fabs(deg[0] - expectedFirst) < 0.01f);
    CHECK(deg[0] < 0 && deg[4] > 0);

    // A smaller circle bends the same text further.
    float tight[5];
    layout::arcAngles(adv, 5, 97, 4, tight);
    CHECK(std::fabs(tight[4] - deg[4] * 2.0f) < 0.01f);

    // One letter sits dead centre.
    float one[1];
    const int a1[] = {30};
    layout::arcAngles(a1, 1, 194, 4, one);
    CHECK(std::fabs(one[0]) < 0.001f);
}
