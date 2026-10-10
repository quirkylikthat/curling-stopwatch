// SleepMemory: keeps the measurement history alive across deep sleep.
//
// Deep sleep powers down normal RAM, so anything in it is lost and the device
// reboots on wake. A small block of "RTC memory" stays powered, though. We
// pack the history into it just before sleeping and unpack it after waking.
//
// A cold power-on or reset leaves no valid snapshot (checksum fails), so the
// history only survives *sleep* -- not switching the device off.
#pragma once

#include "../Config.h"
#include "../core/RingBuffer.h"
#include "../core/Types.h"

namespace curl {
namespace sleepmem {

using HistoryBuffer = RingBuffer<Measurement, config::kHistorySize>;

void save(const HistoryBuffer& history, uint32_t nextSequence);

// Restores and then invalidates the snapshot (one-shot). Returns false if
// there was nothing valid to restore.
bool restore(HistoryBuffer& history, uint32_t& nextSequence);

// Invalidate any snapshot (called on every boot that is NOT a deep-sleep wake,
// so a reset can never resurrect old history).
void discard();

}  // namespace sleepmem
}  // namespace curl
