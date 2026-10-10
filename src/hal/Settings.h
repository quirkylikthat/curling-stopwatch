// Settings: values that survive a power cycle, stored in ESP32 NVS flash.
// Only written when something changes (flash has limited write cycles).
//
// Besides the timing mode it stores opaque blobs (e.g. the user's menu
// settings); the caller owns their format, versioning and validation.
#pragma once

#include <Preferences.h>
#include <stddef.h>
#include <stdint.h>

namespace curl {

class Settings {
public:
    void begin();
    uint8_t profileIndex() const { return profileIndex_; }
    void setProfileIndex(uint8_t index);

    // Blob storage. load() returns false (and leaves `data` untouched) if the
    // key is missing or the stored size differs (e.g. after a format change).
    bool loadBlob(const char* key, void* data, size_t len);
    void saveBlob(const char* key, const void* data, size_t len);

private:
    Preferences prefs_;
    bool ok_ = false;
    uint8_t profileIndex_ = 0;
};

}  // namespace curl
