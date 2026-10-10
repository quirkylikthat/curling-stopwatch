// ViewBuilder: turns application state into a ViewModel.
// Pure logic (no Arduino), unit-tested in test/.
#pragma once

#include "../Config.h"
#include "../core/RingBuffer.h"
#include "../core/SequenceTimer.h"
#include "../core/Types.h"
#include "../ui/ViewModel.h"
#include "Menu.h"
#include "UserSettings.h"

namespace curl {

using History = RingBuffer<Measurement, config::kHistorySize>;

struct ViewInputs {
    const SequenceTimer& timer;
    const TimingProfile& profile;
    const History& history;
    Micros now;
    int8_t batteryPercent;  // -1 = unknown
    bool charging;
    bool timedOut;     // last run timed out with nothing captured
    bool showHistory;  // history screen instead of the timing screen
    // Split bands in use (kSplitBandCount entries, from the user's settings);
    // nullptr = the Config.h defaults.
    const Band* splitBands;
    // The menu to show, or nullptr = not in the menu. `user` is required then.
    const Menu* menu;
    const UserSettings* user;
};

void buildView(const ViewInputs& in, ViewModel& out);

// Exposed for tests / serial logging. `splitBands` as in ViewInputs.
const char* intervalName(IntervalKind kind);
const Band* bandFor(IntervalKind kind, float seconds, const Band* splitBands = nullptr);

}  // namespace curl
