#include "Format.h"

#include <stdio.h>

namespace curl {

char* formatSeconds(Micros duration, int decimals, char* buf, size_t len) {
    if (!buf || len == 0) return buf;
    if (decimals < 1) decimals = 1;
    if (decimals > 2) decimals = 2;

    constexpr Micros kMax = 999990000ULL;  // 999.99 s
    if (duration > kMax) duration = kMax;

    const unsigned long whole = static_cast<unsigned long>(duration / kMicrosPerSecond);
    const Micros remainder = duration % kMicrosPerSecond;

    if (decimals == 1) {
        snprintf(buf, len, "%lu.%01lu", whole, static_cast<unsigned long>(remainder / 100000ULL));
    } else {
        snprintf(buf, len, "%lu.%02lu", whole, static_cast<unsigned long>(remainder / 10000ULL));
    }
    return buf;
}

}  // namespace curl
