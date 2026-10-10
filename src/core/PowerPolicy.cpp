#include "PowerPolicy.h"

namespace curl {

PowerLevel powerLevelFor(Millis idleMs, const PowerTimeouts& t, bool sleepAllowed) {
    if (sleepAllowed) {
        if (t.deepSleepAfter && idleMs >= t.deepSleepAfter) return PowerLevel::DeepSleep;
        if (t.lightSleepAfter && idleMs >= t.lightSleepAfter) return PowerLevel::LightSleep;
    }
    if (t.dimAfter && idleMs >= t.dimAfter) return PowerLevel::Dim;
    return PowerLevel::Active;
}

Millis lightSleepBudget(Millis idleMs, const PowerTimeouts& t) {
    if (!t.deepSleepAfter) return 0;
    if (idleMs >= t.deepSleepAfter) return 1;  // due now; wake immediately
    return t.deepSleepAfter - idleMs;
}

}  // namespace curl
