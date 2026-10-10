# Changelog

Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Versions follow `kFirmwareVersion` in `src/Config.h`.

## [Unreleased]

### Added
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
- Default active brightness changed from 200 to 180.
- **Status LED:** now off while awake and green while asleep. Before this it was always on. (D18)
- **Blue single click** now acts about 0.35 s after release, while it waits to rule out a double-click. (D19)

## [0.1.0] - 2026-10-09

### Added
- **Timing modes:** split, hog-to-hog, and split + hog-to-hog. Taps are interrupt-timestamped.
- **Split weight bands**, as placeholders.
- **Display and feedback:** history row, battery arc, haptics, and dim after 1 min idle.
- **Persistence and output:** the mode is remembered in NVS, and results are printed to the serial port as CSV.
- **Tests and docs:** host unit tests, plus architecture and hardware docs.
