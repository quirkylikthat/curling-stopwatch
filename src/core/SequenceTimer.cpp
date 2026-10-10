#include "SequenceTimer.h"

namespace curl {

namespace {
// Timestamps from the ISR can be a hair newer than the `now` the main loop
// sampled, so never let a subtraction underflow.
inline Micros safeDiff(Micros later, Micros earlier) {
    return later > earlier ? later - earlier : 0;
}
}  // namespace

SequenceTimer::SequenceTimer(Micros minTapGap) : minTapGap_(minTapGap) {}

void SequenceTimer::setProfile(const TimingProfile& profile, uint8_t profileIndex) {
    profile_ = &profile;
    profileIndex_ = profileIndex;
    cancel();
}

void SequenceTimer::cancel() {
    state_ = State::Idle;
    gatesCaptured_ = 0;
}

SequenceTimer::TapResult SequenceTimer::onTap(Micros t) {
    if (!profile_) return TapResult::Ignored;

    if (haveLastTap_ && (t < lastAcceptedTap_ || t - lastAcceptedTap_ < minTapGap_)) {
        return TapResult::Ignored;
    }

    if (state_ != State::Running) {
        // Idle or Finished: this tap opens a new run at gate 0.
        gates_[0] = t;
        gatesCaptured_ = 1;
        state_ = State::Running;
        haveLastTap_ = true;
        lastAcceptedTap_ = t;
        return TapResult::Started;
    }

    const IntervalSpec& spec = profile_->intervals[gatesCaptured_ - 1];
    if (safeDiff(t, gates_[gatesCaptured_ - 1]) < secondsToMicros(spec.minSeconds)) {
        return TapResult::Ignored;
    }

    gates_[gatesCaptured_++] = t;
    haveLastTap_ = true;
    lastAcceptedTap_ = t;

    if (gatesCaptured_ >= profile_->gateCount) {
        finish(true);
        return TapResult::Finished;
    }
    return TapResult::GateRecorded;
}

SequenceTimer::TickResult SequenceTimer::tick(Micros now) {
    if (state_ != State::Running || !profile_) return TickResult::None;

    const IntervalSpec& spec = profile_->intervals[gatesCaptured_ - 1];
    if (safeDiff(now, gates_[gatesCaptured_ - 1]) <= secondsToMicros(spec.maxSeconds)) {
        return TickResult::None;
    }

    if (gatesCaptured_ >= 2) {
        // e.g. Full profile: split captured but the stone never reached the far
        // hog (or the timer forgot to tap). Keep what we have.
        finish(false);
        return TickResult::TimedOutPartial;
    }
    cancel();
    return TickResult::TimedOutEmpty;
}

IntervalKind SequenceTimer::currentIntervalKind() const {
    if (!profile_) return IntervalKind::Split;
    return profile_->intervals[currentInterval()].kind;
}

Micros SequenceTimer::currentIntervalElapsed(Micros now) const {
    if (state_ != State::Running) return 0;
    return safeDiff(now, gates_[gatesCaptured_ - 1]);
}

Micros SequenceTimer::completedDuration(uint8_t i) const {
    if (state_ != State::Running || i + 1 >= gatesCaptured_) return 0;
    return gates_[i + 1] - gates_[i];
}

Measurement SequenceTimer::takeMeasurement() {
    pending_ = false;
    return last_;
}

void SequenceTimer::finish(bool complete) {
    Measurement m;
    m.sequence = nextSequence_++;
    m.profileIndex = profileIndex_;
    m.intervalCount = gatesCaptured_ - 1;
    for (uint8_t i = 0; i < m.intervalCount; ++i) {
        m.kinds[i] = profile_->intervals[i].kind;
        m.durations[i] = gates_[i + 1] - gates_[i];
    }
    m.complete = complete;

    last_ = m;
    pending_ = true;
    state_ = State::Finished;
    gatesCaptured_ = 0;
}

}  // namespace curl
