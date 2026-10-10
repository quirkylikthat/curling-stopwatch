// UserSettings: the values the user can change on the device (Settings menu),
// persisted in NVS by hal/Settings. Pure logic, unit-tested.
//
// Split-band limits are stored in hundredths of a second (integers): edits are
// in exact 0.05 s steps and comparisons never suffer float rounding.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../Config.h"
#include "../core/Classifier.h"

namespace curl {

constexpr size_t kSplitBandCount = sizeof(config::kSplitBands) / sizeof(config::kSplitBands[0]);
constexpr size_t kSplitLimitCount = kSplitBandCount - 1;  // the last band is open-ended

struct UserSettings {
    uint8_t brightness;                        // active screen brightness, 0-255
    uint16_t splitLimitsCs[kSplitLimitCount];  // upper limit of each split band, 1/100 s
};

namespace settings {

constexpr uint8_t kBrightnessMin = config::kBrightnessMin;
constexpr int kLimitStepCs = 5;   // one -/+ tap = 0.05 s (about one tap's hand-timing error)
constexpr int kLimitMinCs = 150;  // 1.50 s: same window as the split sanity check ...
constexpr int kLimitMaxCs = 800;  // 8.00 s: ... in config::kProfiles
constexpr int kLimitGapCs = 5;    // neighbouring limits stay >= 0.05 s apart

UserSettings defaults();  // values from Config.h
bool isValid(const UserSettings& s);

// Edits clamp to the allowed range and keep the limits in order.
// Each returns true if the settings actually changed.
bool setBrightness(UserSettings& s, int value);
bool adjustLimit(UserSettings& s, size_t index, int deltaCs);
bool shiftAllLimits(UserSettings& s, int deltaCs);  // moves every limit together
bool resetSplitLimits(UserSettings& s);

uint8_t brightnessPercent(uint8_t brightness);  // for display: 0-100

// Config labels/colours + the user's limits, ready for classify().
struct SplitBands {
    Band bands[kSplitBandCount];
};
SplitBands makeSplitBands(const UserSettings& s);

}  // namespace settings
}  // namespace curl
