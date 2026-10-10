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

v0.1.0 builds cleanly against board package 3.3.9, M5Unified 0.2.25 and M5GFX 0.2.32. It uses about 600 KB of flash (19%) and 29 KB of static RAM.

## Unit tests (no device needed)

The timing logic in `src/core/` and the screen logic in `src/app/ViewBuilder.cpp` are plain C++. They have tests that run on a PC:

```bash
make -C test
```

On Windows this needs g++, from WSL or MSYS2.

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
├── test/                host unit tests (make -C test)
└── docs/
    ├── ARCHITECTURE.md  design and the reasons behind each decision
    └── HARDWARE.md      pin map, M5Unified API notes, gotchas
```

Read [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) before adding features.
