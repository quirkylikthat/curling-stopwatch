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

bool Settings::loadBlob(const char* key, void* data, size_t len) {
    if (!ok_ || !prefs_.isKey(key) || prefs_.getBytesLength(key) != len) return false;
    return prefs_.getBytes(key, data, len) == len;
}

void Settings::saveBlob(const char* key, const void* data, size_t len) {
    if (ok_) prefs_.putBytes(key, data, len);
}

}  // namespace curl
