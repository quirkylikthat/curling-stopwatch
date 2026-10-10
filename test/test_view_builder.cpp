#include "../src/app/ViewBuilder.h"
#include "TestFramework.h"

using namespace curl;

namespace {

constexpr Micros s(double seconds) { return static_cast<Micros>(seconds * 1e6 + 0.5); }

ViewModel build(const SequenceTimer& t, uint8_t profile, const History& h, Micros now,
                bool timedOut = false, bool showHistory = false) {
    ViewModel vm;
    buildView({t, config::kProfiles[profile], h, now, 80, false, timedOut, showHistory}, vm);
    return vm;
}

// Times one throw on profile `p` (0 split, 1 hog-to-hog, 2 both) and stores it.
void throwStone(SequenceTimer& t, History& h, uint8_t p, double startS, double first,
                double second = 0) {
    t.setProfile(config::kProfiles[p], p);
    t.onTap(s(startS));
    t.onTap(s(startS + first));
    if (second > 0) t.onTap(s(startS + first + second));
    h.push(t.takeMeasurement());
}

}  // namespace

// ---- History screen -----------------------------------------------------------

TEST(history_screen_columns_and_rows) {
    SequenceTimer t(config::kMinTapGapUs);
    History h;
    throwStone(t, h, 0, 0, 3.82);          // #1 split only
    throwStone(t, h, 1, 20, 14.31);        // #2 hog-to-hog only
    throwStone(t, h, 2, 60, 3.95, 14.10);  // #3 both, same throw

    ViewModel vm = build(t, 0, h, s(100), false, true);
    CHECK(vm.screen == ViewModel::Screen::History);
    CHECK_STR(vm.modeName, "LAST 10");
    CHECK_EQ(vm.gateCount, 0);
    CHECK_EQ(vm.rowCount, 3);

    // Newest first.
    CHECK_STR(vm.rows[0].number, "#3");
    CHECK_STR(vm.rows[0].split, "3.95");
    CHECK_STR(vm.rows[0].hogToHog, "14.10");  // same row: recorded together
    CHECK(vm.rows[0].splitColor != 0);

    CHECK_STR(vm.rows[1].split, "-");
    CHECK_EQ(vm.rows[1].splitColor, 0);
    CHECK_STR(vm.rows[1].hogToHog, "14.31");

    CHECK_STR(vm.rows[2].number, "#1");
    CHECK_STR(vm.rows[2].split, "3.82");
    CHECK_STR(vm.rows[2].hogToHog, "-");
}

TEST(history_screen_split_colour_matches_band) {
    SequenceTimer t(config::kMinTapGapUs);
    History h;
    throwStone(t, h, 0, 0, 3.0);  // TAKEOUT band
    ViewModel vm = build(t, 0, h, s(10), false, true);
    CHECK_EQ(vm.rows[0].splitColor, bandFor(IntervalKind::Split, 3.0f)->color);
}

TEST(history_screen_shows_only_newest_ten) {
    SequenceTimer t(config::kMinTapGapUs);
    History h;
    for (int i = 0; i < 14; ++i) throwStone(t, h, 0, i * 20.0, 3.5 + i * 0.01);
    ViewModel vm = build(t, 0, h, s(400), false, true);
    CHECK_EQ(vm.rowCount, config::kHistoryScreenRows);
    CHECK_STR(vm.rows[0].number, "#14");
    CHECK_STR(vm.rows[9].number, "#5");
}

TEST(history_screen_timed_out_throw_keeps_split) {
    SequenceTimer t(config::kMinTapGapUs);
    History h;
    t.setProfile(config::kProfiles[2], 2);
    t.onTap(s(0));
    t.onTap(s(3.9));
    t.tick(s(3.9 + 40));  // never reached the far hog
    h.push(t.takeMeasurement());
    ViewModel vm = build(t, 2, h, s(50), false, true);
    CHECK_STR(vm.rows[0].split, "3.90");
    CHECK_STR(vm.rows[0].hogToHog, "-");
}

TEST(history_screen_empty_and_while_running) {
    SequenceTimer t(config::kMinTapGapUs);
    t.setProfile(config::kProfiles[0], 0);
    History h;
    ViewModel empty = build(t, 0, h, 0, false, true);
    CHECK(empty.screen == ViewModel::Screen::History);
    CHECK_EQ(empty.rowCount, 0);

    // A run in progress doesn't change the history view (no 10 fps redraws).
    t.onTap(s(1));
    CHECK(build(t, 0, h, s(2.0), false, true) == build(t, 0, h, s(5.0), false, true));
}

TEST(view_ready_screen) {
    SequenceTimer t(config::kMinTapGapUs);
    t.setProfile(config::kProfiles[0], 0);
    History h;
    ViewModel vm = build(t, 0, h, 0);
    CHECK(vm.screen == ViewModel::Screen::Ready);
    CHECK_STR(vm.modeName, "SPLIT");
    CHECK_STR(vm.hint, "TAP AT BACK LINE");
    CHECK_STR(vm.main.time, "0.00");
    CHECK_EQ(vm.gateCount, 2);
    CHECK_EQ(vm.gatesDone, 0);
    CHECK_EQ(vm.batteryPercent, 80);
}

TEST(view_ready_shows_timeout) {
    SequenceTimer t(config::kMinTapGapUs);
    t.setProfile(config::kProfiles[0], 0);
    History h;
    CHECK_STR(build(t, 0, h, 0, true).hint, "TIMED OUT");
}

TEST(view_running_split_shows_live_tenths) {
    SequenceTimer t(config::kMinTapGapUs);
    t.setProfile(config::kProfiles[0], 0);
    History h;
    t.onTap(s(10));
    ViewModel vm = build(t, 0, h, s(12.37));
    CHECK(vm.screen == ViewModel::Screen::Running);
    CHECK_STR(vm.main.time, "2.3");
    CHECK_STR(vm.hint, "NEXT: NEAR HOG");
    CHECK_STR(vm.main.caption, "");  // single-interval profile: mode name says it all
    CHECK_STR(vm.main.band, "");     // no band while still running
    CHECK_EQ(vm.gatesDone, 1);
}

TEST(view_result_split_has_band) {
    SequenceTimer t(config::kMinTapGapUs);
    t.setProfile(config::kProfiles[0], 0);
    History h;
    t.onTap(s(10));
    t.onTap(s(13.82));
    h.push(t.takeMeasurement());
    ViewModel vm = build(t, 0, h, s(14));
    CHECK(vm.screen == ViewModel::Screen::Result);
    CHECK_STR(vm.main.time, "3.82");
    CHECK_STR(vm.main.band, "DRAW");
    CHECK_STR(vm.hint, "");
    CHECK_EQ(vm.historyCount, 0);  // the only entry is on screen already
    CHECK_EQ(vm.gatesDone, 2);
}

TEST(view_full_profile_running_shows_split_as_secondary) {
    SequenceTimer t(config::kMinTapGapUs);
    t.setProfile(config::kProfiles[2], 2);
    History h;
    t.onTap(s(0));
    t.onTap(s(3.8));
    ViewModel vm = build(t, 2, h, s(8.85));
    CHECK_STR(vm.main.time, "5.0");
    CHECK_STR(vm.main.caption, "HOG-HOG");
    CHECK_STR(vm.hint, "NEXT: FAR HOG");
    CHECK(vm.hasSecondary);
    CHECK_STR(vm.secondary.caption, "SPLIT");
    CHECK_STR(vm.secondary.time, "3.80");
    CHECK_STR(vm.secondary.band, "DRAW");
}

TEST(view_full_profile_result) {
    SequenceTimer t(config::kMinTapGapUs);
    t.setProfile(config::kProfiles[2], 2);
    History h;
    t.onTap(s(0));
    t.onTap(s(3.8));
    t.onTap(s(18.13));
    h.push(t.takeMeasurement());
    ViewModel vm = build(t, 2, h, s(19));
    CHECK_STR(vm.main.time, "14.33");
    CHECK_STR(vm.main.caption, "HOG-HOG");
    CHECK_STR(vm.secondary.time, "3.80");
}

TEST(view_history_newest_first_skipping_current_result) {
    SequenceTimer t(config::kMinTapGapUs);
    t.setProfile(config::kProfiles[0], 0);
    History h;
    const double splits[] = {3.10, 3.95, 4.20, 3.70};
    double start = 0;
    for (double sp : splits) {
        t.onTap(s(start));
        t.onTap(s(start + sp));
        h.push(t.takeMeasurement());
        start += 20;
    }
    ViewModel vm = build(t, 0, h, s(start));
    CHECK_STR(vm.main.time, "3.70");
    CHECK_EQ(vm.historyCount, 3);
    CHECK_STR(vm.history[0], "4.20");
    CHECK_STR(vm.history[1], "3.95");
    CHECK_STR(vm.history[2], "3.10");
}

TEST(view_equality_detects_changes) {
    SequenceTimer t(config::kMinTapGapUs);
    t.setProfile(config::kProfiles[0], 0);
    History h;
    t.onTap(s(1));
    CHECK(build(t, 0, h, s(2.01)) == build(t, 0, h, s(2.05)));  // same tenth -> no redraw
    CHECK(build(t, 0, h, s(2.01)) != build(t, 0, h, s(2.15)));
}
