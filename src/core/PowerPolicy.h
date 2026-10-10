// PowerPolicy: decides how deeply the device should be asleep, given how long
// it has been idle. Pure logic -- the App acts on the answer, the HAL does the
// hardware work. Unit-tested in test/test_power.cpp.
//
//   idle:   0 ........ dim ........ light sleep ........ deep sleep
//           Active     Dim          LightSleep           DeepSleep
//
// "Idle" means no button input and no timing run in progress.
#pragma once

#include "Types.h"

namespace curl {

enum class PowerLevel : uint8_t {
    Active,      // full brightness
    Dim,         // screen dimmed, everything running
    LightSleep,  // CPU paused, RAM kept, screen off; any button resumes instantly
    DeepSleep,   // almost everything off; any button reboots, history restored
};

struct PowerTimeouts {
    Millis dimAfter;         // 0 = never dim
    Millis lightSleepAfter;  // 0 = never light sleep
    Millis deepSleepAfter;   // 0 = never deep sleep
};

// sleepAllowed = false limits the result to Active/Dim (e.g. on USB power,
// if configured that way).
PowerLevel powerLevelFor(Millis idleMs, const PowerTimeouts& t, bool sleepAllowed);

// How long a light sleep entered at `idleMs` may last before deep sleep is
// due (the timer wake-up). 0 = no limit (deep sleep disabled).
Millis lightSleepBudget(Millis idleMs, const PowerTimeouts& t);

}  // namespace curl
