// Config.h -- every tunable in one place.
//
// If you want to change *behaviour* (thresholds, timings, colours, which modes
// exist), you should only need to edit this file. Logic lives elsewhere.
//
// This header is pure C++ (no Arduino includes) so the unit tests can use it.
#pragma once

#include <stdint.h>

#include "core/Classifier.h"
#include "core/Types.h"

namespace curl {
namespace config {

constexpr const char* kFirmwareVersion = "0.1.0";

// --- Colours (RGB565) ------------------------------------------------------
constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// --- Pins (from the StopWatch pin map) -------------------------------------
// KEYA (yellow) = G2, KEYB (blue) = G1. We read these GPIOs directly rather
// than via M5.BtnA/BtnB so timing never depends on M5Unified's button mapping
// or polling rate.
constexpr int kPinTimingButton = 2;       // yellow: tap at each line
constexpr int kPinControlButton = 1;      // blue: mode / cancel / clear
constexpr bool kButtonsActiveLow = true;  // board has pull-ups; pressed = LOW

// true  = timestamp the yellow button in a GPIO interrupt (sub-millisecond).
// false = fall back to polling in loop() (accuracy limited by loop/render time).
// Flip to false if interrupts misbehave on your hardware revision.
constexpr bool kUseInterruptTiming = true;

// --- Input timing -----------------------------------------------------------
constexpr Millis kControlDebounceMs = 25;
constexpr Millis kControlHoldMs = 800;  // blue long-press threshold
// Blue double-click window. A single click is only acted on once this has
// passed without a second press, so mode change / cancel react ~0.3 s later.
constexpr Millis kControlDoubleClickMs = 300;
constexpr Micros kTimingIsrLockoutUs = 150000;  // ignore edges for 150 ms after a press (bounce)
constexpr Micros kMinTapGapUs = 400000;         // ignore any tap < 0.4 s after the previous one

// --- Timing profiles ----------------------------------------------------------
// Order here = order the blue button cycles through.
// min/max are sanity windows: taps faster than min are ignored, and a run with
// no tap for longer than max times out.
// clang-format off
constexpr TimingProfile kProfiles[] = {
    {"Split", "SPLIT", 2,
     {"BACK LINE", "NEAR HOG"},
     {{IntervalKind::Split, 1.5f, 8.0f}}},
    {"Hog to hog", "HOG-HOG", 2,
     {"NEAR HOG", "FAR HOG"},
     {{IntervalKind::HogToHog, 5.0f, 35.0f}}},
    {"Split + hog to hog", "SPLIT+H2H", 3,
     {"BACK LINE", "NEAR HOG", "FAR HOG"},
     {{IntervalKind::Split, 1.5f, 8.0f}, {IntervalKind::HogToHog, 5.0f, 35.0f}}},
};
// clang-format on
constexpr uint8_t kProfileCount = sizeof(kProfiles) / sizeof(kProfiles[0]);

// --- Weight bands -------------------------------------------------------------
// !! PLACEHOLDER VALUES -- calibrate on your own ice and stones !!
// Split = back line -> near hog line, in seconds. Shorter = harder throw.
// Reference point: on nominal ice a ~3.8 s split corresponds to a ~14.3 s
// hog-to-hog draw; on very keen (Olympic) ice draws run ~4.1 s / ~15 s.
// Bands must be in ascending maxSeconds order; last band is open-ended.
// clang-format off
constexpr Band kSplitBands[] = {
    {2.90f, "PEEL", rgb565(255, 60, 60)},
    {3.20f, "TAKEOUT", rgb565(255, 140, 0)},
    {3.50f, "BOARD", rgb565(255, 210, 0)},
    {3.75f, "HACK", rgb565(180, 230, 60)},
    {4.15f, "DRAW", rgb565(60, 220, 120)},
    {kBandOpenEnd, "GUARD", rgb565(80, 190, 255)},
};
// clang-format on

// Hog-to-hog bands are intentionally empty for now: the time itself ("ice is
// running 14") is what players use. Add bands here once calibrated.
constexpr Band kHogToHogBands[] = {
    {kBandOpenEnd, "", rgb565(200, 200, 200)},
};

// --- History ------------------------------------------------------------------
constexpr uint8_t kHistorySize = 32;        // measurements kept in RAM
constexpr uint8_t kHistoryShown = 3;        // previous results shown under the main time
constexpr uint8_t kHistoryScreenRows = 10;  // throws on the history screen (blue double-click)

// --- Feedback -----------------------------------------------------------------
// You are watching the stone, not the screen, so every accepted tap buzzes.
constexpr uint8_t kHapticStrength = 200;  // 0-255
constexpr Millis kHapticTapMs = 35;       // tap accepted
constexpr Millis kHapticResultMs = 70;    // result ready (played twice)
constexpr Millis kHapticGapMs = 80;
constexpr Millis kHapticCancelMs = 250;  // cancel / timeout

constexpr bool kBeepEnabled = false;  // speaker off by default (it's loud on the ice)
constexpr uint16_t kBeepHz = 3000;
constexpr Millis kBeepMs = 30;
constexpr uint8_t kSpeakerVolume = 96;  // 0-255

// --- Display / power ----------------------------------------------------------
constexpr Millis kFrameIntervalMs = 33;  // ~30 fps cap while values change
constexpr Millis kBatteryPollMs = 5000;
constexpr uint8_t kBrightnessActive = 180;  // 0-255
constexpr uint8_t kBrightnessDim = 30;

// --- Power saving -------------------------------------------------------------
// Idle = no button input and no timing run in progress. Stages (0 = disabled):
//   dim -> light sleep (screen off, instant resume) -> deep sleep (reboot on wake)
// For bench testing, try 20 s / 40 s / 80 s here.
constexpr Millis kDimAfterMs = 60UL * 1000UL;                // 1 min
constexpr Millis kLightSleepAfterMs = 10UL * 60UL * 1000UL;  // 10 min
constexpr Millis kDeepSleepAfterMs = 30UL * 60UL * 1000UL;   // 30 min
//constexpr Millis kDimAfterMs = 20UL * 1000UL;                // 20 seconds 
//constexpr Millis kLightSleepAfterMs = 40UL * 1000UL;  // 40 seconds
//constexpr Millis kDeepSleepAfterMs = 60UL * 1000UL;   // 60 seconds

// Yellow press that wakes from LIGHT sleep also counts as a timing tap (wake-up
// takes ~1 ms, so the split is still accurate). After DEEP sleep the device has
// to reboot (~1 s), so that press only wakes it.
constexpr bool kLightSleepWakeTapCounts = true;

// false = never sleep while USB power is present (handy while developing:
// sleeping drops the serial monitor). Dimming still happens.
constexpr bool kSleepOnUsbPower = true;
constexpr int kUsbPresentMillivolts = 4000;  // VBUS above this = on USB power

// Green status LED (the board's only LED; single colour, on/off only, driven by
// the M5PM1). Default: off while awake (the screen shows it's on), a short flash
// every 5 s in light sleep, off in deep sleep (the CPU is off, so it can't flash
// there, and solid would cost ~0.55 mA). Powered off = dark too.
enum class LedMode : uint8_t { Off, On, Flash };
constexpr bool kStatusLedWhenAwake = false;
constexpr LedMode kStatusLedLightSleep = LedMode::Flash;
constexpr bool kStatusLedInDeepSleep = false;
constexpr Millis kStatusLedFlashPeriodMs = 5000;  // one flash every 5 s ...
constexpr Millis kStatusLedFlashOnMs = 50;        // ... lit for 50 ms

// --- Unused hardware, kept off to save battery -------------------------------
// Motion sensor (BMI270). M5Unified would start accel + gyro (~0.7 mA); we don't
// use it, so it's held in suspend (~3.5 uA). true = let M5Unified start it.
constexpr bool kImuEnabled = false;
// 5 V output on the Grove port (M5PM1 boost converter). Nothing is plugged in,
// so the booster stays off. Set true before connecting a Grove unit.
constexpr bool kGrovePortPower = false;

// Pause at the end of every loop() so the CPU core can idle (WAITI) instead of
// spinning at 240 MHz. Timing isn't affected: taps are timestamped in the ISR.
constexpr Millis kLoopIdleMs = 1;

}  // namespace config
}  // namespace curl
