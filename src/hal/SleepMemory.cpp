#include "SleepMemory.h"

#include <esp_attr.h>
#include <string.h>

#include "../core/HistorySnapshot.h"

namespace curl {
namespace sleepmem {

namespace {

using Snapshot = HistorySnapshot<config::kHistorySize>;

// Raw bytes on purpose: a plain zero-initialised byte array is never touched
// by C++ start-up code, so whatever deep sleep preserved is still there when
// we look. (An object with a constructor could be re-initialised at boot.)
RTC_DATA_ATTR alignas(8) uint8_t gBlob[sizeof(Snapshot)];

static_assert(sizeof(Snapshot) < 4096, "history snapshot too large for RTC memory");

}  // namespace

void save(const HistoryBuffer& history, uint32_t nextSequence) {
    Snapshot s;
    packSnapshot(history, nextSequence, s);
    memcpy(gBlob, static_cast<const void*>(&s), sizeof(s));
}

bool restore(HistoryBuffer& history, uint32_t& nextSequence) {
    Snapshot s;
    memcpy(static_cast<void*>(&s), gBlob, sizeof(s));
    const bool ok = unpackSnapshot(s, history, nextSequence);
    discard();  // one-shot: never restore the same snapshot twice
    return ok;
}

void discard() { memset(gBlob, 0, sizeof(gBlob)); }

}  // namespace sleepmem
}  // namespace curl
