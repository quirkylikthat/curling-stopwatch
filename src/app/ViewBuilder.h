// ViewBuilder: turns application state into a ViewModel.
// Pure logic (no Arduino), unit-tested in test/.
#pragma once

#include "../Config.h"
#include "../core/RingBuffer.h"
#include "../core/SequenceTimer.h"
#include "../core/Types.h"
#include "../ui/ViewModel.h"

namespace curl {

using History = RingBuffer<Measurement, config::kHistorySize>;

struct ViewInputs {
    const SequenceTimer& timer;
    const TimingProfile& profile;
    const History& history;
    Micros now;
    int8_t batteryPercent;  // -1 = unknown
    bool charging;
    bool timedOut;  // last run timed out with nothing captured
};

void buildView(const ViewInputs& in, ViewModel& out);

// Exposed for tests / serial logging.
const char* intervalName(IntervalKind kind);
const Band* bandFor(IntervalKind kind, float seconds);

}  // namespace curl
