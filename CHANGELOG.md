# Changelog

Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Versions follow `kFirmwareVersion` in `src/Config.h`.

## [Unreleased]

## [0.2.0] - 2026-10-10

All items below verified on the device (bench test), except the serial CSV output; see `docs/STATUS.md`.

### Added
- **Menu** (2 s blue hold; touchscreen; D21):
  - **Settings → Brightness:** a live slider.
  - **Settings → Split bands:** −/+ per limit, SHIFT ALL, RESET.
  - **Export:** placeholder.
  - **Clear history:** with confirmation.
  - Settings are saved in NVS and survive power-off.
- **History screen** (blue double-click): the last 10 throws, with split on the left (coloured by band) and hog-to-hog on the right, on the same row when both were timed on that throw. Yellow jumps back and times; a blue click goes back. (D19)
- **Power saving.** After 1 min idle the screen dims. After 10 min the device enters light sleep, which wakes instantly; a yellow wake press counts as a tap. After 30 min it enters deep sleep; it wakes on either button and the history is restored. (ARCHITECTURE D17)
- **VS Code / PlatformIO build** alongside the Arduino IDE, with pinned versions: pioarduino 55.03.39, M5Unified 0.2.25, M5GFX 0.2.32. (D16)
- **Quality gates:**
  - `scripts/verify.sh` (with `verify.ps1` for Windows).
  - GitHub Actions CI.
  - `-Werror` on our sources.
  - `.clang-format` and `.editorconfig`.
- **Troubleshooting:** `scripts/pio-diagnose.ps1` resets the toolchain cache and captures a build log.
- **Context for AI agents:** `AGENTS.md`, `CLAUDE.md`, and `docs/STATUS.md`.

### Changed
- **Clear history** moved from the 0.8 s blue hold into the menu. A blue hold is now 2 s and opens the menu. (D21)
- Default active brightness changed from 200 to 180.
- **Status LED:** now off while awake and green while asleep. Before this it was always on. (D18)
- **Blue single click** now acts about 0.3 s after release, while it waits to rule out a double-click. (D19)
- **Status LED in sleep:** now a 50 ms flash every 5 s in light sleep, and off in deep sleep. It was solid green. (D20)
- **Battery trims** (D20):
  - The motion sensor is held in suspend.
  - The Grove port's 5 V booster is off. Set `kGrovePortPower = true` before using a Grove unit.
  - The main loop now idles 1 ms per pass.

### Fixed
- **Menu BACK taps often didn't register.** Taps now use our own detection, with a 20 px wobble allowed and the tap placed where the finger landed. Hit zones are padded, and BACK accepts taps down to the screen edge. (D22)
- **Split-bands page was cramped.** It now has a scrolling list with larger 64×50 px −/+ buttons and a fixed header and footer. (D22)
- **SHIFT ALL was easy to miss into BACK** at the bottom of the list. There's now blank space after the last row and a dead gap above the footer, about 60 px clear in total. (D22)
- **Timing screen spacing:** the weight label ("DRAW") was touching the big number. It and the SPLIT+H2H secondary line are now spaced evenly between the big number and the history row, using measured font ink extents (`ui/Layout.h`, `scripts/fontmetrics.py`).

## [0.1.0] - 2026-10-09

### Added
- **Timing modes:** split, hog-to-hog, and split + hog-to-hog. Taps are interrupt-timestamped.
- **Split weight bands**, as placeholders.
- **Display and feedback:** history row, battery arc, haptics, and dim after 1 min idle.
- **Persistence and output:** the mode is remembered in NVS, and results are printed to the serial port as CSV.
- **Tests and docs:** host unit tests, plus architecture and hardware docs.
