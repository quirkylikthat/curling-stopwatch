# Curling Timer — M5Stack StopWatch

Firmware for the [M5Stack StopWatch](https://docs.m5stack.com/en/core/StopWatch) (SKU C152) that turns it into a curling split timer.

**v0.1.0 modes** (blue button cycles through them):

| Mode | Taps (yellow) | Measures |
|---|---|---|
| SPLIT | back line → near hog | Delivery split, with a weight band (GUARD / DRAW / HACK / BOARD / TAKEOUT / PEEL) |
| HOG-HOG | near hog → far hog | Stone speed / ice speed ("ice is running 14") |
| SPLIT+H2H | back line → near hog → far hog | Both, from one stone |

> The weight bands in `src/Config.h` are **placeholders**. Calibrate them on your own ice.

## Controls

| Input | Action |
|---|---|
| Yellow, tap | Start, then stop, at each line. Every accepted tap buzzes. |
| Blue, click | While timing: cancel (false start). Otherwise: next mode. |
| Blue, hold (0.8 s) | Clear history |
| Power, click / double-click | Reset / power off (hardware) |

Results also go out over USB serial at 115200 baud as `csv,...` lines, for logging.

## Power saving

"Idle" means no button pressed and no timing run in progress.

| Idle for | What happens | To resume |
|---|---|---|
| 1 min | Screen dims | Any button. A yellow press still counts as a tap. |
| 10 min | **Light sleep**: screen off, CPU paused, everything kept in memory | Any button, instant. A yellow press **also starts timing**, as if the screen had been on. |
| 30 min | **Deep sleep**: nearly everything off | Either button. The device reboots in about 1 s, and **the history is kept**. The press only wakes it. |

All times are in `src/Config.h` (`kDimAfterMs`, `kLightSleepAfterMs`, `kDeepSleepAfterMs`; 0 disables a stage).

- **To switch off completely**, double-press the power button. History is lost.
- **While sleeping, the USB serial monitor disconnects.** To stop the device sleeping while plugged into USB during development, set `kSleepOnUsbPower = false`.
- **To try it on the bench**, temporarily set the three times to about 20 s, 40 s and 80 s.

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

What's pinned: [pioarduino](https://github.com/pioarduino/platform-espressif32) platform **55.03.39**, which is Arduino-ESP32 **3.3.9**, the same core as M5Stack's board package 3.3.9. Also **M5Unified 0.2.25** and **M5GFX 0.2.32**. To upgrade, change the versions in `platformio.ini`, then re-run the tests and a device check.

PlatformIO creates `.vscode/` and `.pio/` itself. Both are git-ignored.

**Known issues (pioarduino 55.03.39 on Windows):**
- **Don't use *Verbose Build* (`pio run -v`).** The platform crashes while printing the `firmware.bin` step (`TypeError: ... '_Null' and 'str'`), even though compiling and linking succeeded. Normal *Build* works.
- **If you see `MissingPackageManifestError`, a toolchain install was interrupted.** Close VS Code, then run `scripts\pio-diagnose.ps1 -Reset`. It clears PlatformIO's downloaded toolchain, rebuilds, and writes `pio-build.log`.
- **Optional:** enable Windows long-path support for faster builds. PlatformIO prints the one-line admin command for this.

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

v0.1.0 builds with zero warnings from our code under both toolchains: board package 3.3.9 / pioarduino 55.03.39, M5Unified 0.2.25, M5GFX 0.2.32. It uses about 600–650 KB of flash (~20%) and about 30 KB of static RAM.

## Unit tests (no device needed)

The timing logic in `src/core/` and the screen logic in `src/app/ViewBuilder.cpp` are plain C++, with 41 tests that run on a PC. Either command runs the same tests:

```bash
pio run -e native -t tests    # PlatformIO
make -C test                  # plain make + g++
```

## Note on OneDrive

This folder lives in OneDrive. PlatformIO writes build output to `.pio/` (100+ MB), and OneDrive will try to sync it. OneDrive can also occasionally interfere with git's files. Once the repo is pushed to GitHub, consider moving the folder somewhere OneDrive doesn't sync, such as `C:\dev\stopwatch`. Both tools open it from anywhere.

## Project layout

```
stopwatch/
├── stopwatch.ino        thin entry point: setup() / loop() -> App
├── src/
│   ├── Config.h         every tunable: pins, modes, bands, haptics, display
│   ├── core/            pure C++ timing logic, unit-tested, no Arduino
│   ├── hal/             hardware: buttons + ISR timing, haptics, battery, NVS
│   ├── ui/              ViewModel + Renderer for the 466x466 round AMOLED
│   └── app/             App (event routing) + ViewBuilder (state -> ViewModel)
├── test/                host unit tests
├── docs/
│   ├── ARCHITECTURE.md  design and the reasons behind each decision
│   └── HARDWARE.md      pin map, M5Unified API notes, gotchas
│
│   PlatformIO only (Arduino IDE ignores these):
├── platformio.ini       envs: stopwatch (device), native (tests); pinned versions
├── boards/              m5stack-stopwatch.json board definition
├── variants/            StopWatch pin map (copied from M5Stack's board package)
└── scripts/             pio_tests.py adds the "Run unit tests" task
```

Read [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) before adding features.
