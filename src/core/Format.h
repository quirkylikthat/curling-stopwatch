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

// Result log over USB serial: this header once at boot, then one line per result.
constexpr const char* kResultCsvHeader = "csv,seq,profile,split_s,hog_to_hog_s,complete";

// One result line, e.g. "csv,12,SPLIT+H2H,3.86,14.21,1". Times use formatSeconds
// (2 decimals, truncated), so the log always matches the screen. A time field is
// empty when that interval wasn't timed; the last field is 0 for a timed-out run.
// Returns buf.
char* formatResultCsv(const Measurement& m, const char* profileName, char* buf, size_t len);

}  // namespace curl
