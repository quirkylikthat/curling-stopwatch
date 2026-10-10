// Classifier: maps a measured time onto a named band (e.g. a split of 3.85 s
// -> "DRAW"). Band tables live in Config.h so they can be calibrated per ice
// without touching logic.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace curl {

struct Band {
    float maxSeconds;   // band applies while seconds < maxSeconds; ascending order
    const char* label;  // short, upper-case, fits on screen ("DRAW")
    uint16_t color;     // RGB565; opaque to core, used by the UI
};

constexpr float kBandOpenEnd = 1.0e9f;  // use as maxSeconds for the last band

// Returns the matching band, or nullptr if the table is empty.
const Band* classify(const Band* table, size_t count, float seconds);

template <size_t N>
const Band* classify(const Band (&table)[N], float seconds) {
    return classify(table, N, seconds);
}

}  // namespace curl
