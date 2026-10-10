#include "Settings.h"

namespace curl {

namespace {
constexpr const char* kNamespace = "curltimer";  // NVS namespace (max 15 chars)
constexpr const char* kKeyProfile = "profile";
}  // namespace

void Settings::begin() {
    ok_ = prefs_.begin(kNamespace, /*readOnly=*/false);
    if (ok_) profileIndex_ = prefs_.getUChar(kKeyProfile, 0);
}

void Settings::setProfileIndex(uint8_t index) {
    if (index == profileIndex_) return;
    profileIndex_ = index;
    if (ok_) prefs_.putUChar(kKeyProfile, index);
}

}  // namespace curl
