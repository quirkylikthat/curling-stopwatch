// SequenceTimer: the heart of the stopwatch.
//
// A small state machine that turns timestamped taps into interval measurements
// according to a TimingProfile (see Types.h). It knows nothing about buttons,
// screens or clocks -- callers pass timestamps in. That makes it trivial to
// unit-test and lets the HAL capture timestamps as precisely as it can (ISR).
//
//   Idle --tap--> Running --tap--> ... --last gate--> Finished --tap--> Running
//                   |                                    ^
//                   +---- timeout (>=1 interval done) ---+
//                   +---- timeout (nothing captured) ----> Idle
#pragma once

#include "Types.h"

namespace curl {

class SequenceTimer {
public:
    enum class State : uint8_t { Idle, Running, Finished };

    enum class TapResult : uint8_t {
        Ignored,       // too soon after the previous tap (bounce / double tap)
        Started,       // first gate captured, run in progress
        GateRecorded,  // intermediate gate captured (3-gate profiles)
        Finished,      // last gate captured, measurement ready
    };

    enum class TickResult : uint8_t {
        None,
        TimedOutEmpty,    // timed out before any interval was captured -> Idle
        TimedOutPartial,  // timed out after >=1 interval -> partial measurement ready
    };

    // minTapGap: any tap closer than this to the previously *accepted* tap is
    // ignored in every state. Guards against switch bounce on release and
    // accidental double taps right after finishing a run.
    explicit SequenceTimer(Micros minTapGap);

    // Switch profile. Cancels any run in progress.
    void setProfile(const TimingProfile& profile, uint8_t profileIndex);

    TapResult onTap(Micros t);
    TickResult tick(Micros now);
    void cancel();  // abandon the current run -> Idle

    State state() const { return state_; }
    bool isRunning() const { return state_ == State::Running; }

    // Live display helpers (only meaningful while Running).
    uint8_t currentInterval() const { return gatesCaptured_ ? gatesCaptured_ - 1 : 0; }
    IntervalKind currentIntervalKind() const;
    Micros currentIntervalElapsed(Micros now) const;
    // Intervals already completed in the current run (for 3-gate profiles).
    uint8_t completedIntervals() const { return gatesCaptured_ ? gatesCaptured_ - 1 : 0; }
    Micros completedDuration(uint8_t i) const;

    // A measurement becomes available after Finished / TimedOutPartial.
    bool hasMeasurement() const { return pending_; }
    Measurement takeMeasurement();  // returns it and clears the pending flag
    const Measurement& lastMeasurement() const { return last_; }

    // Sequence numbering survives deep sleep (restored from a snapshot) so the
    // serial CSV log never repeats a number within a session.
    uint32_t nextSequence() const { return nextSequence_; }
    void setNextSequence(uint32_t next) { nextSequence_ = next ? next : 1; }

private:
    void finish(bool complete);

    const TimingProfile* profile_ = nullptr;
    uint8_t profileIndex_ = 0;
    Micros minTapGap_;

    State state_ = State::Idle;
    Micros gates_[kMaxGates] = {};
    uint8_t gatesCaptured_ = 0;

    bool haveLastTap_ = false;
    Micros lastAcceptedTap_ = 0;

    uint32_t nextSequence_ = 1;
    Measurement last_;
    bool pending_ = false;
};

}  // namespace curl
