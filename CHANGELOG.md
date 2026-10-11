# Changelog

Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Versions follow `kFirmwareVersion` in `src/Config.h`.

## [Unreleased]

### Added
- **Screenshot tool** (`tools/screenshots`, `make -C tools/screenshots`): renders every screen the README describes to a PNG in `docs/screenshots/` on a PC, round with transparent corners like the glass (31 images: splash, the three modes ready / running / result, all six weight bands, timed-out, battery arc states, history screen full and empty, every menu page). It compiles the firmware's real `Renderer` unchanged and drives the real view/menu logic, so no firmware code changed. See decision D23.

- **License:** the project is now under the PolyForm Noncommercial License 1.0.0 (`LICENSE`). Noncommercial use, modification and sharing are allowed; sharing must keep the `Required Notice:` attribution line in `NOTICE`. Commercial use needs a separate license (contact in README).

- **Release workflow** (`.github/workflows/release.yml`): pushing a `vX.Y.Z` tag builds the firmware and creates a draft GitHub Release with one flashable image (`scripts/merge_firmware.py`, with tests), the app-only binary, `SHA256SUMS`, `LICENSE` and `NOTICE`. It refuses a tag that doesn't match `kFirmwareVersion` or a commit that failed CI. README gains an Install section.

- **Dependabot** (`.github/dependabot.yml`): one grouped monthly pull request for newer GitHub Actions versions, checked by CI.

### Changed
- **First full pin review** (recorded in `docs/PINS.md`): all firmware libraries and CI tools are current except pioarduino (three patch releases behind; scheduled as a batch with an on-device re-test).
- **Monthly pin review:** new `docs/PINS.md` lists every pinned version and where it's set, with a review checklist and log. AGENTS.md tells agents to check its *Last reviewed* date at session start and raise it with Mike when it's over about 35 days old.
- **Workflows moved off Node 20 and `ubuntu-latest`:** checkout v7, setup-python v7, cache v5 and upload-artifact v7 run on Node 24 natively (clears the Node 20 deprecation warning). Both workflows run on `ubuntu-24.04` instead of `ubuntu-latest`, so the label's move to Ubuntu 26.04 (from 19 Oct 2026) can't change the build unannounced. `arduino/setup-arduino-cli@v2` has no Node 24 release yet and still shows the warning in CI.
- **README install steps:** esptool now finds the watch by itself, and a new "Finding the serial port" section covers Windows, macOS and Linux.
- **CI hardening:** PlatformIO Core (6.2.0) and arduino-cli (1.5.1) are now pinned like the rest of the toolchain; the workflow token is read-only and the checkout doesn't keep credentials; runs trigger on pushes to `main`, pull requests, or manually, and a newer run cancels an older one on the same branch.
- **Docs:** the README is now the user guide (summary, features, controls, screens, menu, power saving) and an index of the project docs. Build setup, testing, tunables and project layout moved to the new `docs/DEVELOPMENT.md`, which also gains a releasing checklist.

### Fixed
- **USB log rounded times while the screen truncates them.** A 3.456 s split showed 3.45 on the watch but logged 3.46. The CSV line now uses the same truncating formatter as the screen (`formatResultCsv` in `core/Format`, unit-tested).

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
