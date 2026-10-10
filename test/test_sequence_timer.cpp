#include "../src/Config.h"
#include "../src/core/SequenceTimer.h"
#include "TestFramework.h"

using namespace curl;

namespace {

constexpr Micros s(double seconds) { return static_cast<Micros>(seconds * 1e6 + 0.5); }

const TimingProfile& kSplit = config::kProfiles[0];
const TimingProfile& kH2H = config::kProfiles[1];
const TimingProfile& kFull = config::kProfiles[2];

SequenceTimer makeTimer(const TimingProfile& p, uint8_t idx = 0) {
    SequenceTimer t(config::kMinTapGapUs);
    t.setProfile(p, idx);
    return t;
}

}  // namespace

TEST(split_two_taps_produce_measurement) {
    auto t = makeTimer(kSplit);
    CHECK(t.state() == SequenceTimer::State::Idle);
    CHECK(t.onTap(s(10.0)) == SequenceTimer::TapResult::Started);
    CHECK(t.isRunning());
    CHECK(t.onTap(s(13.82)) == SequenceTimer::TapResult::Finished);
    CHECK(t.state() == SequenceTimer::State::Finished);
    CHECK(t.hasMeasurement());

    Measurement m = t.takeMeasurement();
    CHECK(!t.hasMeasurement());
    CHECK_EQ(m.intervalCount, 1);
    CHECK(m.kinds[0] == IntervalKind::Split);
    CHECK_EQ(m.durations[0], s(3.82));
    CHECK(m.complete);
    CHECK_EQ(m.sequence, 1u);
}

TEST(tap_inside_min_gap_is_ignored) {
    auto t = makeTimer(kSplit);
    t.onTap(s(1.0));
    CHECK(t.onTap(s(1.1)) == SequenceTimer::TapResult::Ignored);  // bounce
    CHECK(t.isRunning());
}

TEST(tap_below_interval_minimum_is_ignored) {
    auto t = makeTimer(kSplit);  // split min = 1.5 s
    t.onTap(s(1.0));
    CHECK(t.onTap(s(2.2)) == SequenceTimer::TapResult::Ignored);
    CHECK(t.onTap(s(4.8)) == SequenceTimer::TapResult::Finished);
    CHECK_EQ(t.takeMeasurement().durations[0], s(3.8));
}

TEST(timeout_with_nothing_captured_returns_to_idle) {
    auto t = makeTimer(kSplit);  // split max = 8 s
    t.onTap(s(1.0));
    CHECK(t.tick(s(8.9)) == SequenceTimer::TickResult::None);
    CHECK(t.tick(s(9.1)) == SequenceTimer::TickResult::TimedOutEmpty);
    CHECK(t.state() == SequenceTimer::State::Idle);
    CHECK(!t.hasMeasurement());
}

TEST(full_profile_three_taps) {
    auto t = makeTimer(kFull, 2);
    CHECK(t.onTap(s(0.5)) == SequenceTimer::TapResult::Started);
    CHECK(t.onTap(s(4.3)) == SequenceTimer::TapResult::GateRecorded);
    CHECK_EQ(t.completedIntervals(), 1);
    CHECK_EQ(t.completedDuration(0), s(3.8));
    CHECK(t.currentIntervalKind() == IntervalKind::HogToHog);
    CHECK(t.onTap(s(18.63)) == SequenceTimer::TapResult::Finished);

    Measurement m = t.takeMeasurement();
    CHECK_EQ(m.intervalCount, 2);
    CHECK_EQ(m.profileIndex, 2);
    CHECK_EQ(m.durations[0], s(3.8));
    CHECK_EQ(m.durations[1], s(14.33));
    CHECK_EQ(m.find(IntervalKind::Split), 0);
    CHECK_EQ(m.find(IntervalKind::HogToHog), 1);
    CHECK_NEAR(m.seconds(1), 14.33, 1e-4);
}

TEST(full_profile_timeout_keeps_partial_split) {
    auto t = makeTimer(kFull, 2);
    t.onTap(s(0.0));
    t.onTap(s(3.9));
    CHECK(t.tick(s(3.9 + 35.5)) == SequenceTimer::TickResult::TimedOutPartial);
    CHECK(t.state() == SequenceTimer::State::Finished);
    Measurement m = t.takeMeasurement();
    CHECK_EQ(m.intervalCount, 1);
    CHECK(!m.complete);
    CHECK_EQ(m.durations[0], s(3.9));
    CHECK_EQ(m.find(IntervalKind::HogToHog), -1);
}

TEST(release_bounce_after_finish_does_not_start_new_run) {
    auto t = makeTimer(kSplit);
    t.onTap(s(1.0));
    t.onTap(s(4.8));
    CHECK(t.onTap(s(5.0)) == SequenceTimer::TapResult::Ignored);
    CHECK(t.state() == SequenceTimer::State::Finished);
    CHECK(t.onTap(s(6.0)) == SequenceTimer::TapResult::Started);
}

TEST(sequence_numbers_increment) {
    auto t = makeTimer(kH2H, 1);
    t.onTap(s(0));
    t.onTap(s(14));
    CHECK_EQ(t.takeMeasurement().sequence, 1u);
    t.onTap(s(20));
    t.onTap(s(34));
    CHECK_EQ(t.takeMeasurement().sequence, 2u);
}

TEST(cancel_and_profile_change_reset_run) {
    auto t = makeTimer(kSplit);
    t.onTap(s(1.0));
    t.cancel();
    CHECK(t.state() == SequenceTimer::State::Idle);

    t.onTap(s(3.0));
    t.setProfile(kH2H, 1);
    CHECK(t.state() == SequenceTimer::State::Idle);
}

TEST(live_elapsed_never_underflows) {
    auto t = makeTimer(kSplit);
    t.onTap(s(5.0));
    // ISR timestamp can be slightly newer than the loop's `now`.
    CHECK_EQ(t.currentIntervalElapsed(s(4.999)), 0u);
    CHECK_EQ(t.currentIntervalElapsed(s(6.25)), s(1.25));
    CHECK(t.tick(s(4.999)) == SequenceTimer::TickResult::None);
}

TEST(no_profile_ignores_taps) {
    SequenceTimer t(config::kMinTapGapUs);
    CHECK(t.onTap(s(1)) == SequenceTimer::TapResult::Ignored);
    CHECK(t.tick(s(100)) == SequenceTimer::TickResult::None);
}
