# Developer guide

How to build, flash, test and change the firmware. For what the device does and how to use it, see the [README](../README.md).

Before changing code, read [AGENTS.md](../AGENTS.md) and [ARCHITECTURE.md](ARCHITECTURE.md). This applies whether you're a person or an AI model. All commands below run from the project folder.

## Two ways to build

The same folder builds with either tool, and both produce the same firmware from the same sources. Use whichever you like, and switch any time.

| | VS Code + PlatformIO (recommended) | Arduino IDE |
|---|---|---|
| Board package + library versions | Pinned in `platformio.ini` | Whatever is installed globally |
| Board settings (PSRAM, flash, USB) | In the repo (`boards/m5stack-stopwatch.json`) | *Tools* menu, per machine |
| Unit tests | Sidebar task: *native → Custom → Run unit tests* | Separate `make -C test` |
| Editor | Full IntelliSense, go-to-definition, refactoring | Basic |

## Setup (VS Code + PlatformIO)

1. Install [VS Code](https://code.visualstudio.com/), then the **PlatformIO IDE** extension (Extensions panel, search "PlatformIO").
2. *File → Open Folder…* and open the `stopwatch` folder. PlatformIO detects `platformio.ini`. The **first build downloads the toolchain and framework, about 3.5 GB on disk** in `%USERPROFILE%\.platformio`, outside the project. That happens once.
3. Use the status-bar buttons, or the PlatformIO sidebar (ant icon) → **stopwatch**:
   - ✓ **Build**
   - → **Upload**. Put the device in download mode first: hold the power button about 2 s until the green LED lights.
   - 🔌 **Monitor**: serial at 115200, with crash backtraces decoded automatically.
4. **Unit tests:** sidebar → **native → Custom → Run unit tests**, or `pio run -e native -t tests`.
   These run on your PC, so they need a host `g++` on `PATH`. On Windows, install one from MSYS2 or WinLibs. On macOS, the Xcode command-line tools provide one.

What's pinned: [pioarduino](https://github.com/pioarduino/platform-espressif32) platform **55.03.39**, which is Arduino-ESP32 **3.3.9**, the same core as M5Stack's board package 3.3.9. Also **M5Unified 0.2.25** and **M5GFX 0.2.32**. To upgrade, change the versions in `platformio.ini`, `.github/workflows/ci.yml` and this file together, then re-run the tests and a device check.

PlatformIO creates `.vscode/` and `.pio/` itself. Both are git-ignored.

## Setup (Arduino IDE 2.x)

1. **Board package.** *File → Preferences → Additional boards manager URLs*, add
   `https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json`.
   Then in *Boards Manager*, search **M5Stack** and install **3.3.7 or newer**. 3.3.9 was the latest at time of writing.
2. **Libraries.** In *Library Manager*, install **M5Unified** (≥ 0.2.25) and **M5GFX** (≥ 0.2.31). Accept any dependencies it offers.
   StopWatch support first appeared in M5Unified 0.2.16, and later releases fixed audio and display init on this board.
3. **Board.** *Tools → Board → M5Stack → **M5StopWatch***. Keep the default *Tools* options: **PSRAM: OPI PSRAM**, Flash 16 MB, USB CDC on boot enabled. The display canvas needs PSRAM.
4. **Open** `stopwatch.ino`. The folder must stay named `stopwatch`, because Arduino requires the folder name to match the `.ino` file.
5. **Download mode.** Plug in USB-C. Hold the power button about 2 s until the green LED lights, then release.
6. Select the port and click **Upload**.

### Command line (optional)

```bash
arduino-cli config add board_manager.additional_urls https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
arduino-cli core update-index
arduino-cli core install m5stack:esp32
arduino-cli lib install M5Unified M5GFX
arduino-cli compile --fqbn m5stack:esp32:m5stack_stopwatch .
arduino-cli upload  --fqbn m5stack:esp32:m5stack_stopwatch -p COM5 .   # your port
```

v0.2.0 builds with zero warnings from our code under both toolchains: board package 3.3.9 / pioarduino 55.03.39, M5Unified 0.2.25, M5GFX 0.2.32. It uses about 600–650 KB of flash (~20%) and about 30 KB of static RAM.

## Unit tests (no device needed)

The timing logic in `src/core/` and the pure app logic in `src/app/` (screen content, settings, menu) are plain C++, with 73 tests that run on a PC. Either command runs the same tests:

```bash
pio run -e native -t tests    # PlatformIO
make -C test                  # plain make + g++
```

Add a test as a `TEST(name)` in any `test/test_*.cpp` file; it's picked up automatically.

## Checking a change

```bash
scripts/verify.sh             # format, unit tests, PlatformIO + Arduino builds (CI runs this)
scripts/verify.sh --quick     # format + unit tests only
```

On Windows: `powershell -ExecutionPolicy Bypass -File scripts\verify.ps1` runs the tests (if `g++` is installed) and the PlatformIO build.

**CI:** GitHub Actions runs `scripts/verify.sh` on every push to `main` (`.github/workflows/ci.yml`). Formatting uses clang-format 23.1.3 (`.clang-format`).

The full definition of done (tests, docs, decision log, STATUS, CHANGELOG) is in [AGENTS.md §3](../AGENTS.md).

## Serial monitor

115200 baud. At boot the firmware prints a header, then one line per result:

```
csv,seq,profile,split_s,hog_to_hog_s,complete
csv,12,SPLIT+H2H,3.86,14.21,1
```

Empty fields mean that interval wasn't timed. `complete` is 0 for a run that timed out. Menu taps are also logged as `[menu] tap x,y`, which helps when tuning touch targets.

**While the device sleeps, the serial monitor disconnects.** To stop it sleeping while plugged into USB, set `kSleepOnUsbPower = false` in `src/Config.h`.

## Tuning in `src/Config.h`

Every tunable lives in `src/Config.h`; logic lives elsewhere. The ones you're most likely to touch:

| Setting | What it controls |
|---|---|
| `kDimAfterMs`, `kLightSleepAfterMs`, `kDeepSleepAfterMs` | Power-saving stages (0 disables one). For bench testing, try about 20 s / 40 s / 80 s; commented bench values sit just below. Don't commit them. |
| `kSleepOnUsbPower` | `false` = never sleep on USB power (keeps the serial monitor alive) |
| `kProfiles` | The timing modes, their gate names, and sanity windows (min/max seconds) |
| `kSplitBands` | **Default** weight bands. Users override them on the device (Menu → Settings → Split bands). |
| `kControlHoldMs`, `kControlDoubleClickMs` | Blue-button hold (menu) and double-click (history) timing |
| `kMinTapGapUs`, `kTimingIsrLockoutUs` | Yellow-button bounce and double-tap protection |
| `kStatusLedWhenAwake`, `kStatusLedLightSleep`, `kStatusLedInDeepSleep` | Green status LED behaviour |
| `kImuEnabled`, `kGrovePortPower` | Unused hardware kept off. **Set `kGrovePortPower = true` before plugging in a Grove unit.** |
| `kBrightnessActive`, `kBrightnessDim`, `kBrightnessMin` | Default and limit brightness (users change active brightness in the menu) |
| `kBeepEnabled`, `kHaptic*` | Speaker (off by default) and vibration patterns |

## Releasing

1. Set `kFirmwareVersion` in `src/Config.h` (shown on the boot splash).
2. In `CHANGELOG.md`, rename *Unreleased* to the version and date, and start a new empty *Unreleased*.
3. Run `scripts/verify.sh`, commit "Release vX.Y.Z", and create an annotated tag: `git tag -a vX.Y.Z -m "vX.Y.Z"`.
4. Push both: `git push Github main --follow-tags`.

## Project layout

```
stopwatch/
├── stopwatch.ino        thin entry point: setup() / loop() -> App
├── src/
│   ├── Config.h         every tunable: pins, modes, bands, haptics, display, power
│   ├── core/            pure C++ logic, unit-tested, no Arduino:
│   │                    SequenceTimer, ButtonLogic, Classifier, PowerPolicy, Format, ...
│   ├── hal/             hardware: buttons + ISR timing, touch, sleep, haptics, NVS, RTC memory
│   ├── ui/              ViewModel, Renderer, Theme, Layout, MenuLayout (466x466 round AMOLED)
│   └── app/             App (event routing) + pure ViewBuilder, Menu, UserSettings
├── test/                host unit tests (Makefile + test_*.cpp)
├── docs/
│   ├── DEVELOPMENT.md   this file
│   ├── ARCHITECTURE.md  design and the reasons behind each decision (D1–D22)
│   ├── HARDWARE.md      pin map, M5Unified API notes, gotchas
│   └── STATUS.md        what's verified on the device vs. only compiled/tested
├── AGENTS.md            rules + definition of done for AI agents and humans
├── CLAUDE.md            imports AGENTS.md (for Claude)
├── CHANGELOG.md
├── scripts/             verify.sh/.ps1, pio-diagnose.ps1, pio_tests.py, fontmetrics.py
├── .github/workflows/   CI: runs scripts/verify.sh
│
│   PlatformIO only (Arduino IDE ignores these):
├── platformio.ini       envs: stopwatch (device), native (tests); pinned versions
├── boards/              m5stack-stopwatch.json board definition
└── variants/            StopWatch pin map (copied from M5Stack's board package)
```

## Known issues and environment notes

- **Don't use *Verbose Build* (`pio run -v`).** pioarduino 55.03.39 crashes while printing the `firmware.bin` step (`TypeError: ... '_Null' and 'str'`), even though compiling and linking succeeded. Normal *Build* works.
- **`MissingPackageManifestError` means a toolchain install was interrupted.** Close VS Code, then run `scripts\pio-diagnose.ps1 -Reset`. It clears PlatformIO's downloaded toolchain, rebuilds, and writes `pio-build.log`.
- **Optional:** enable Windows long-path support for faster builds. PlatformIO prints the one-line admin command for this.
- **Keep the project out of OneDrive** (it lives in `C:\dev\stopwatch`). PlatformIO's `.pio/` build folder is 100+ MB, and OneDrive syncing can interfere with git.
- More pitfalls, already paid for once, are in [AGENTS.md §5–6](../AGENTS.md).
