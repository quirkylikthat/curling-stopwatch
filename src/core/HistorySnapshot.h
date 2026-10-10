// HistorySnapshot: packs the measurement history into a flat, checksummed
// block so it can survive deep sleep (the HAL keeps the block in RTC memory,
// which stays powered while the rest of the chip is off).
//
// Pure logic, unit-tested. The checksum guards against garbage after a cold
// boot or a crash; a snapshot that fails it is simply ignored.
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "RingBuffer.h"
#include "Types.h"

namespace curl {

template <size_t N>
struct HistorySnapshot {
    static constexpr uint32_t kMagic = 0x31525543;  // "CUR1"

    uint32_t magic;
    uint32_t nextSequence;
    uint32_t count;
    Measurement items[N];  // oldest first
    uint32_t checksum;
};

namespace detail {
// FNV-1a over every byte before the checksum field.
template <size_t N>
uint32_t snapshotChecksum(const HistorySnapshot<N>& s) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&s);
    const size_t len = offsetof(HistorySnapshot<N>, checksum);
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < len; ++i) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}
}  // namespace detail

template <size_t N>
void packSnapshot(const RingBuffer<Measurement, N>& history, uint32_t nextSequence,
                  HistorySnapshot<N>& out) {
    memset(static_cast<void*>(&out), 0, sizeof(out));  // deterministic padding for the checksum
    out.magic = HistorySnapshot<N>::kMagic;
    out.nextSequence = nextSequence;
    out.count = static_cast<uint32_t>(history.size());
    for (size_t i = 0; i < history.size(); ++i) {
        // history[0] is newest; store oldest first so restore can just push.
        memcpy(&out.items[i], &history[history.size() - 1 - i], sizeof(Measurement));
    }
    out.checksum = detail::snapshotChecksum(out);
}

// Returns false (and leaves outputs untouched) if the snapshot is invalid.
template <size_t N>
bool unpackSnapshot(const HistorySnapshot<N>& s, RingBuffer<Measurement, N>& history,
                    uint32_t& nextSequence) {
    if (s.magic != HistorySnapshot<N>::kMagic || s.count > N ||
        s.checksum != detail::snapshotChecksum(s)) {
        return false;
    }
    history.clear();
    for (uint32_t i = 0; i < s.count; ++i) {
        Measurement m;
        memcpy(&m, &s.items[i], sizeof(Measurement));
        history.push(m);
    }
    nextSequence = s.nextSequence;
    return true;
}

}  // namespace curl
