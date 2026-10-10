// Settings: values that survive a power cycle, stored in ESP32 NVS flash.
// Only written when something changes (flash has limited write cycles).
#pragma once

#include <Preferences.h>
#include <stdint.h>

namespace curl {

class Settings {
public:
    void begin();
    uint8_t profileIndex() const { return profileIndex_; }
    void setProfileIndex(uint8_t index);

private:
    Preferences prefs_;
    bool ok_ = false;
    uint8_t profileIndex_ = 0;
};

}  // namespace curl
