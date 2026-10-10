// Shared value types for the curling stopwatch.
//
// Everything in src/core/ is plain C++17 with NO Arduino / M5 includes, so it
// can be compiled and unit-tested on a PC (see test/). Time is always passed
// in explicitly as microseconds; core code never reads a clock itself.
#pragma once

#include <stdint.h>

namespace curl {

using Micros = uint64_t;  // esp_timer_get_time() units; 64-bit never wraps in practice
using Millis = uint32_t;

constexpr Micros kMicrosPerSecond = 1000000ULL;
constexpr Micros secondsToMicros(float s) { return static_cast<Micros>(s * 1e6f); }

// What a measured interval represents on the sheet.
enum class IntervalKind : uint8_t {
    Split,     // back line (or tee) -> near hog: delivery weight
    HogToHog,  // near hog -> far hog: stone speed / ice speed
};

// A timing profile is a sequence of "gates" (lines the stone crosses).
// The user taps once per gate; N gates produce N-1 intervals.
//   Split      : 2 gates -> [Split]
//   Hog-to-hog : 2 gates -> [HogToHog]
//   Full       : 3 gates -> [Split, HogToHog]
constexpr uint8_t kMaxGates = 3;
constexpr uint8_t kMaxIntervals = kMaxGates - 1;

struct IntervalSpec {
    IntervalKind kind;
    float minSeconds;  // taps closer together than this are ignored (double-tap / bounce)
    float maxSeconds;  // no tap within this window -> run times out
};

struct TimingProfile {
    const char* name;                 // full name, e.g. "Split"
    const char* shortName;            // shown on screen, e.g. "SPLIT"
    uint8_t gateCount;                // 2..kMaxGates
    const char* gateNames[kMaxGates]; // prompts: "BACK LINE", "NEAR HOG", ...
    IntervalSpec intervals[kMaxIntervals];
};

// One finished (or partially finished) timing run.
struct Measurement {
    uint32_t sequence = 0;       // increments for every measurement since boot
    uint8_t profileIndex = 0;    // which TimingProfile produced it
    uint8_t intervalCount = 0;   // how many intervals were actually captured
    IntervalKind kinds[kMaxIntervals] = {};
    Micros durations[kMaxIntervals] = {};
    bool complete = false;       // false = timed out after at least one interval

    float seconds(uint8_t i) const {
        return i < intervalCount ? static_cast<float>(durations[i]) / 1e6f : 0.0f;
    }
    // Returns the index of the first interval of `kind`, or -1.
    int find(IntervalKind kind) const {
        for (uint8_t i = 0; i < intervalCount; ++i) {
            if (kinds[i] == kind) return i;
        }
        return -1;
    }
};

// Hardware-independent input events. The HAL translates buttons/touch into these.
enum class InputType : uint8_t {
    TimingTap,     // yellow button: precise timestamp captured in an ISR
    ControlClick,  // blue button short press
    ControlHold,   // blue button long press
};

struct InputEvent {
    InputType type;
    Micros timestamp;
};

}  // namespace curl
