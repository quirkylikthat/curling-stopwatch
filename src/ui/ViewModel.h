// ViewModel: everything the screen shows, as plain data.
//
// The App builds one of these each frame (see app/ViewBuilder) and the
// Renderer draws it. Nothing in here touches the display, so the "what to
// show" logic is unit-tested on the PC, and the Renderer only redraws when the
// model actually changes.
#pragma once

#include <stdint.h>
#include <string.h>

namespace curl {

struct Readout {
    char time[12];     // "3.82", "14.3"
    char caption[12];  // interval name when a profile has several ("SPLIT")
    char band[12];     // weight band label ("DRAW"), may be empty
    uint16_t bandColor;
};

// One line of the history screen = one throw (one Measurement).
struct HistoryRow {
    char number[12];      // "#12" (measurement sequence number, matches the serial CSV)
    char split[8];        // "3.82", or "-" if this throw had no split
    uint16_t splitColor;  // weight-band colour; 0 = no split
    char hogToHog[8];     // "14.31", or "-"
};

// Menu pages (Screen::Menu). app/Menu navigates between them.
enum class MenuPage : uint8_t { Main, Settings, Brightness, Bands, Export, ConfirmClear };

struct MenuView {
    static constexpr uint8_t kMaxLimits = 8;
    MenuPage page;
    uint8_t brightness;  // 0-255 (slider position)
    uint8_t brightnessPercent;
    uint8_t limitCount;
    uint16_t limitsCs[kMaxLimits];  // split-band upper limits, 1/100 s
    uint8_t historyCount;           // for "clear N results?"
    int16_t bandsScroll;            // px the split-band list is scrolled down
};

struct ViewModel {
    enum class Screen : uint8_t { Ready, Running, Result, History, Menu };
    static constexpr uint8_t kMaxHistoryRows = 10;

    Screen screen;
    char modeName[16];  // "SPLIT", "HOG-HOG", ...
    char hint[24];      // "TAP AT BACK LINE", "NEXT: NEAR HOG", "TIMED OUT"
    uint8_t gateCount;  // progress dots
    uint8_t gatesDone;

    Readout main;       // big number
    bool hasSecondary;  // 3-gate profiles: the other interval
    Readout secondary;

    uint8_t historyCount;
    char history[3][12];  // previous results (headline value only)

    // Screen::History only: newest first.
    uint8_t rowCount;
    HistoryRow rows[kMaxHistoryRows];

    // Screen::Menu only.
    MenuView menu;

    int8_t batteryPercent;  // -1 = unknown
    bool charging;

    // Builders must memset() the struct to zero first so this is reliable.
    bool operator==(const ViewModel& o) const { return memcmp(this, &o, sizeof(*this)) == 0; }
    bool operator!=(const ViewModel& o) const { return !(*this == o); }
};

}  // namespace curl
