// Tests for PowerPolicy, HistorySnapshot, ButtonLogic::suppressUntilRelease and
// SequenceTimer sequence continuity -- the pure parts of the sleep feature.
#include "../src/Config.h"
#include "../src/core/ButtonLogic.h"
#include "../src/core/HistorySnapshot.h"
#include "../src/core/PowerPolicy.h"
#include "../src/core/RingBuffer.h"
#include "../src/core/SequenceTimer.h"
#include "TestFramework.h"

using namespace curl;

namespace {

constexpr Millis kMin = 60UL * 1000UL;
const PowerTimeouts kT{1 * kMin, 10 * kMin, 30 * kMin};

Measurement makeMeasurement(uint32_t seq, Micros split) {
    Measurement m;
    m.sequence = seq;
    m.intervalCount = 1;
    m.kinds[0] = IntervalKind::Split;
    m.durations[0] = split;
    m.complete = true;
    return m;
}

}  // namespace

// ---- PowerPolicy ------------------------------------------------------------

TEST(power_levels_follow_idle_time) {
    CHECK(powerLevelFor(0, kT, true) == PowerLevel::Active);
    CHECK(powerLevelFor(59 * 1000, kT, true) == PowerLevel::Active);
    CHECK(powerLevelFor(1 * kMin, kT, true) == PowerLevel::Dim);
    CHECK(powerLevelFor(10 * kMin - 1, kT, true) == PowerLevel::Dim);
    CHECK(powerLevelFor(10 * kMin, kT, true) == PowerLevel::LightSleep);
    CHECK(powerLevelFor(30 * kMin - 1, kT, true) == PowerLevel::LightSleep);
    CHECK(powerLevelFor(30 * kMin, kT, true) == PowerLevel::DeepSleep);
}

TEST(power_sleep_not_allowed_caps_at_dim) {
    CHECK(powerLevelFor(45 * kMin, kT, false) == PowerLevel::Dim);
}

TEST(power_zero_disables_a_stage) {
    const PowerTimeouts noLight{1 * kMin, 0, 30 * kMin};
    CHECK(powerLevelFor(15 * kMin, noLight, true) == PowerLevel::Dim);
    CHECK(powerLevelFor(30 * kMin, noLight, true) == PowerLevel::DeepSleep);

    const PowerTimeouts noDeep{1 * kMin, 10 * kMin, 0};
    CHECK(powerLevelFor(600 * kMin, noDeep, true) == PowerLevel::LightSleep);
    CHECK_EQ(lightSleepBudget(10 * kMin, noDeep), 0u);  // no timer wake-up
}

TEST(power_light_sleep_budget_wakes_for_deep_sleep) {
    CHECK_EQ(lightSleepBudget(10 * kMin, kT), 20 * kMin);
    CHECK_EQ(lightSleepBudget(25 * kMin, kT), 5 * kMin);
    CHECK_EQ(lightSleepBudget(31 * kMin, kT), 1u);  // overdue: wake at once
}

TEST(power_config_stages_are_ordered) {
    CHECK(config::kDimAfterMs < config::kLightSleepAfterMs);
    CHECK(config::kLightSleepAfterMs < config::kDeepSleepAfterMs);
}

// ---- HistorySnapshot --------------------------------------------------------

TEST(snapshot_round_trip_keeps_order_and_sequence) {
    RingBuffer<Measurement, 4> h;
    h.push(makeMeasurement(7, 3800000));
    h.push(makeMeasurement(8, 3950000));
    h.push(makeMeasurement(9, 4100000));

    HistorySnapshot<4> snap;
    packSnapshot(h, 10, snap);

    RingBuffer<Measurement, 4> restored;
    uint32_t next = 0;
    CHECK(unpackSnapshot(snap, restored, next));
    CHECK_EQ(next, 10u);
    CHECK_EQ(restored.size(), 3u);
    CHECK_EQ(restored[0].sequence, 9u);  // newest first, as before
    CHECK_EQ(restored[2].sequence, 7u);
    CHECK_EQ(restored[1].durations[0], 3950000u);
}

TEST(snapshot_full_ring_round_trip) {
    RingBuffer<Measurement, 3> h;
    for (uint32_t i = 1; i <= 5; ++i) h.push(makeMeasurement(i, i * 1000000));  // wraps
    HistorySnapshot<3> snap;
    packSnapshot(h, 6, snap);
    RingBuffer<Measurement, 3> r;
    uint32_t next = 0;
    CHECK(unpackSnapshot(snap, r, next));
    CHECK_EQ(r.size(), 3u);
    CHECK_EQ(r[0].sequence, 5u);
    CHECK_EQ(r[2].sequence, 3u);
}

TEST(snapshot_rejects_garbage_and_corruption) {
    HistorySnapshot<4> zero;
    memset(static_cast<void*>(&zero), 0, sizeof(zero));  // what RTC memory holds after a cold boot
    RingBuffer<Measurement, 4> r;
    r.push(makeMeasurement(1, 1));
    uint32_t next = 42;
    CHECK(!unpackSnapshot(zero, r, next));
    CHECK_EQ(r.size(), 1u);  // untouched
    CHECK_EQ(next, 42u);

    RingBuffer<Measurement, 4> h;
    h.push(makeMeasurement(3, 3000000));
    HistorySnapshot<4> snap;
    packSnapshot(h, 4, snap);
    snap.items[0].durations[0] += 1;  // flip a bit of payload
    CHECK(!unpackSnapshot(snap, r, next));
}

// ---- ButtonLogic::suppressUntilRelease ----------------------------------------

TEST(button_wake_press_does_not_click) {
    ButtonLogic b(25, 800);
    b.suppressUntilRelease();
    CHECK_EQ(b.update(0, true), ButtonLogic::None);
    CHECK_EQ(b.update(900, true), ButtonLogic::None);  // no Hold either
    b.update(1000, false);
    CHECK_EQ(b.update(1030, false), ButtonLogic::None);  // release swallowed
    // Next real press behaves normally.
    b.update(2000, true);
    CHECK_EQ(b.update(2030, true), ButtonLogic::Pressed);
    b.update(2100, false);
    CHECK_EQ(b.update(2130, false), ButtonLogic::Released | ButtonLogic::Click);
}

TEST(button_suppress_while_already_released_clears_quickly) {
    ButtonLogic b(25, 800);
    b.update(0, false);
    b.suppressUntilRelease();
    CHECK_EQ(b.update(30, false), ButtonLogic::None);
    b.update(100, true);
    CHECK_EQ(b.update(130, true), ButtonLogic::Pressed);
}

// ---- SequenceTimer sequence continuity ------------------------------------------

TEST(sequence_continues_after_restore) {
    SequenceTimer t(config::kMinTapGapUs);
    t.setProfile(config::kProfiles[0], 0);
    t.setNextSequence(17);
    t.onTap(1000000);
    t.onTap(4800000);
    CHECK_EQ(t.takeMeasurement().sequence, 17u);
    CHECK_EQ(t.nextSequence(), 18u);
    t.setNextSequence(0);  // invalid -> 1
    CHECK_EQ(t.nextSequence(), 1u);
}
