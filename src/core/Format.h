// Text formatting helpers (no String / heap use).
#pragma once

#include <stddef.h>

#include "Types.h"

namespace curl {

// Formats a duration as seconds with 1 or 2 decimals: "3.42", "14.1".
// Truncates rather than rounds (stopwatch convention: never show time that
// has not yet elapsed). Values >= 1000 s are clamped to "999.99".
// Returns buf for convenience.
char* formatSeconds(Micros duration, int decimals, char* buf, size_t len);

}  // namespace curl
