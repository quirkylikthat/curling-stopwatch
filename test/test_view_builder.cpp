#include "../src/app/ViewBuilder.h"
#include "TestFramework.h"

using namespace curl;

namespace {

constexpr Micros s(double seconds) { return static_cast<Micros>(seconds * 1e6 + 0.5); }

ViewModel build(const SequenceTimer& t, uint8_t profile, const History& h, Micros now,
                bool timedOut = false) {
    ViewModel vm;
    buildView({t, config::kProfiles[profile], h, now, 80, false, timedOut}, vm);
    return vm;
}

}  // namespace

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
