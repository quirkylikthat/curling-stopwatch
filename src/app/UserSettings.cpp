#include "UserSettings.h"

namespace curl {
namespace settings {

namespace {

int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

int lowerBound(const UserSettings& s, size_t i) {
    return i == 0 ? kLimitMinCs : s.splitLimitsCs[i - 1] + kLimitGapCs;
}

int upperBound(const UserSettings& s, size_t i) {
    return i + 1 == kSplitLimitCount ? kLimitMaxCs : s.splitLimitsCs[i + 1] - kLimitGapCs;
}

}  // namespace

UserSettings defaults() {
    UserSettings s{};
    s.brightness = config::kBrightnessActive;
    for (size_t i = 0; i < kSplitLimitCount; ++i) {
        // + 0.5f: 2.90f is 2.8999998..., don't let it truncate to 289.
        s.splitLimitsCs[i] =
            static_cast<uint16_t>(config::kSplitBands[i].maxSeconds * 100.0f + 0.5f);
    }
    return s;
}

bool isValid(const UserSettings& s) {
    if (s.brightness < kBrightnessMin) return false;
    for (size_t i = 0; i < kSplitLimitCount; ++i) {
        const int v = s.splitLimitsCs[i];
        if (v < kLimitMinCs || v > kLimitMaxCs) return false;
        if (i > 0 && v - s.splitLimitsCs[i - 1] < kLimitGapCs) return false;
    }
    return true;
}

bool setBrightness(UserSettings& s, int value) {
    const uint8_t v = static_cast<uint8_t>(clampInt(value, kBrightnessMin, 255));
    if (v == s.brightness) return false;
    s.brightness = v;
    return true;
}

bool adjustLimit(UserSettings& s, size_t index, int deltaCs) {
    if (index >= kSplitLimitCount) return false;
    const int v =
        clampInt(s.splitLimitsCs[index] + deltaCs, lowerBound(s, index), upperBound(s, index));
    if (v == s.splitLimitsCs[index]) return false;
    s.splitLimitsCs[index] = static_cast<uint16_t>(v);
    return true;
}

bool shiftAllLimits(UserSettings& s, int deltaCs) {
    // Shrink the shift so the whole set stays inside [min, max]; spacing is untouched.
    if (deltaCs > 0) {
        const int room = kLimitMaxCs - s.splitLimitsCs[kSplitLimitCount - 1];
        if (deltaCs > room) deltaCs = room;
    } else if (deltaCs < 0) {
        const int room = kLimitMinCs - s.splitLimitsCs[0];  // <= 0
        if (deltaCs < room) deltaCs = room;
    }
    if (deltaCs == 0) return false;
    for (size_t i = 0; i < kSplitLimitCount; ++i) {
        s.splitLimitsCs[i] = static_cast<uint16_t>(s.splitLimitsCs[i] + deltaCs);
    }
    return true;
}

bool resetSplitLimits(UserSettings& s) {
    const UserSettings d = defaults();
    bool changed = false;
    for (size_t i = 0; i < kSplitLimitCount; ++i) {
        changed |= s.splitLimitsCs[i] != d.splitLimitsCs[i];
        s.splitLimitsCs[i] = d.splitLimitsCs[i];
    }
    return changed;
}

uint8_t brightnessPercent(uint8_t brightness) {
    return static_cast<uint8_t>((brightness * 100 + 127) / 255);
}

SplitBands makeSplitBands(const UserSettings& s) {
    SplitBands out{};
    for (size_t i = 0; i < kSplitBandCount; ++i) {
        out.bands[i] = config::kSplitBands[i];  // label + colour
        if (i < kSplitLimitCount) out.bands[i].maxSeconds = s.splitLimitsCs[i] / 100.0f;
    }
    return out;
}

}  // namespace settings
}  // namespace curl
