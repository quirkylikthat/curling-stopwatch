# AGENTS.md — working on this repo (AI agents and humans)

This file is the contract for anyone changing this code: Claude, another model, or a person. Read it fully before your first edit. `CLAUDE.md` simply imports it.

## 1. What this is

Firmware for the **M5Stack StopWatch** (ESP32-S3, 466×466 round AMOLED, 2 buttons, vibration motor) that times curling stones:
- **Split:** back line to near hog.
- **Hog-to-hog:** near hog to far hog.
- **Both,** from one stone.

The owner is **Mike**. He rates his C++ as "some experience". He wants design decisions explained, and plans to add more curling features over time.

**Read before changing anything:**

| File | Why |
|---|---|
| `docs/ARCHITECTURE.md` | Layers, main loop, **decision log D1–D23**. Don't contradict a decision without adding a new one that supersedes it. |
| `docs/HARDWARE.md` | Pins, M5Unified API facts, sleep and wake details. Some items are verified from library source and some aren't; it says which. |
| `docs/STATUS.md` | What's verified **on the device** versus only compiled or unit-tested. |
| `docs/DEVELOPMENT.md` | Build setup for both toolchains, commands, tunables, releasing. |
| `docs/PINS.md` | Every pinned version, where it's set, and the **monthly review** (check its *Last reviewed* date at session start). |
| `src/Config.h` | Every tunable. |

## 2. Invariants (do not break)

1. **`src/core/` is pure C++17.** No Arduino, M5, ESP-IDF or FreeRTOS includes. Time is always passed in, never read. The same applies to `src/Config.h`, `src/ui/ViewModel.h`, `src/ui/Layout.h`, `src/ui/MenuLayout.h` and the pure app files `src/app/ViewBuilder.*`, `UserSettings.*` and `Menu.*`. This is what lets the unit tests run on a PC. New pure app files must be added to both `test/Makefile` (`PURE_APP`) and `[env:native]` in `platformio.ini`.
2. **Dependency direction:**
   - `core` depends on nothing.
   - `hal` and `ui` may use `core`.
   - `app` uses everything.
   - `ui` must not know `app`, and `hal` must not know `ui`.
3. **Tunables live in `Config.h`.** No magic numbers elsewhere.
4. **`loop()` never blocks.**
   - No `delay()`. The only exceptions are:
     - the boot splash;
     - the brief release-wait before deep sleep;
     - the deliberate **1 ms idle yield at the end of each `loop()`** (`kLoopIdleMs`), which lets the CPU core idle (D20).
   - No busy-waits.
   - Feedback (haptics, beeps) is non-blocking.
5. **No heap or `String` after boot.** Use fixed buffers: `RingBuffer`, `EventFifo`, `char[]`.
6. **Timing accuracy is the product.**
   - The yellow button is timestamped in a GPIO ISR (`esp_timer_get_time()`).
   - Keep the ISR tiny: no `Serial`, no M5 calls, no allocation.
   - Don't switch timing to `M5.BtnA`/`BtnB` or to polling.
   - Use the same button for start and stop.
7. **A timing tap is never lost** to power saving or navigation:
   - Dim: the tap counts.
   - Light-sleep wake by yellow: it counts as the first gate.
   - History screen or menu: yellow jumps back and times.
   - **Touch is only acted on inside the menu** (gloves, accidental contact during play).
8. **Times are truncated, not rounded** (stopwatch convention).
9. **Screen = f(state).**
   - `ViewBuilder` builds a `ViewModel`, and `Renderer` draws it into the PSRAM canvas.
   - Redraw only on change.
   - Black background (AMOLED).
10. **Both toolchains must keep building from the same tree:**
    - Arduino IDE: `stopwatch.ino` plus `src/`. The folder must be named `stopwatch`.
    - PlatformIO: `platformio.ini`, `boards/`, `variants/`.
11. **Versions are pinned:**
    - pioarduino `55.03.39` = Arduino-ESP32 3.3.9
    - M5Stack board package 3.3.9
    - M5Unified `0.2.25`
    - M5GFX `0.2.32`
    - clang-format `23.1.3`
    - PlatformIO Core `6.2.0` and arduino-cli `1.5.1` (CI only)

    Change them only on purpose, everywhere at once (`platformio.ini`, `.github/workflows/ci.yml`, `.github/workflows/release.yml`, `docs/DEVELOPMENT.md`, `docs/PINS.md`), and with a re-test.

    **Monthly review:** every pinned item (including CI actions and the runner image) is listed in `docs/PINS.md` with a *Last reviewed* date. At the start of a session, check that date. If it's more than about 35 days old, tell Mike straight away and offer to run the review (the steps are in that file). Reviewing is not upgrading: look for security fixes, end-of-life and deprecations, and ask before changing any firmware library or toolchain pin.

## 3. Definition of done

A change is done when **all** of these are true:

- [ ] **Pure logic has unit tests** in `test/`. Each test is a `TEST(name)` in a `test_*.cpp` file, picked up automatically.
- [ ] **`scripts/verify.sh` passes:**
  - formatting
  - 76+ unit tests
  - PlatformIO build with `-Werror` on our sources
  - Arduino CLI build with no warnings in our sources
  - On Windows without bash, run `scripts\verify.ps1`; CI runs the full gate.
- [ ] **Code is formatted** with `clang-format -i` (`.clang-format`). Hand-aligned tables in `Config.h` are fenced with `// clang-format off/on`.
- [ ] **Docs are updated:**
  - **ARCHITECTURE decision log:** a new `Dn` entry for any non-obvious choice. Use the template below.
  - **README:** if features, controls or other user-visible behaviour changed. It's the user guide and docs index.
  - **`docs/DEVELOPMENT.md`:** if build setup, tooling, commands or tunables a developer would touch changed.
  - **`docs/STATUS.md`:** add the feature as *not yet verified on device*.
  - **`CHANGELOG.md`:** add an entry under *Unreleased*.
  - **ARCHITECTURE §4 checklist:** add device test steps for anything hardware-dependent.
- [ ] **Commits are small and focused.** The subject line is imperative, and the body explains *why*. Never commit `.pio/`, `pio-build.log`, or `.vscode/` files other than `extensions.json`.

**Never claim something works on the device unless Mike has confirmed it.** Say "builds and passes unit tests; not yet verified on hardware".

**Licensing:** the project is under the PolyForm Noncommercial License 1.0.0 (`LICENSE`), with the attribution line in `NOTICE`. Don't edit either, and don't add code under an incompatible license. Dependencies are downloaded at build time, not vendored. Copying third-party code into the repo needs Mike's approval and its notice added to `NOTICE`.

### Decision log template (append to `docs/ARCHITECTURE.md` §2)

```markdown
### Dn — <short title>
**Choice:** what we did.
**Why:** the reasoning, the alternatives considered, and the numbers/sources if any.
**Revisit if:** the condition that would make us change it.
```

## 4. Commands

```bash
scripts/verify.sh            # full gate (format, tests, both firmware builds)
scripts/verify.sh --quick    # format + unit tests, seconds
make -C test                 # unit tests only (any g++)
pio run -e native -t tests   # unit tests via PlatformIO
pio run -e stopwatch         # firmware build
pio run -e stopwatch -t upload && pio device monitor
```

**Never use `pio run -v`.** pioarduino 55.03.39 crashes in verbose mode at the `firmware.bin` step (`TypeError ... '_Null'`), even though the build itself succeeded.

## 5. Environment pitfalls (already paid for once)

- **`MissingPackageManifestError`** means a pioarduino toolchain install was interrupted. pioarduino keeps extra state in `~/.platformio/tools`, `dist` and `idf-env.json`. Fix it with `scripts/pio-diagnose.ps1 -Reset`, which writes `pio-build.log`.
- **The project lives at `C:\dev\stopwatch`.** It was moved out of OneDrive, which synced `.pio/` and interfered with git.
- **Windows long-path support is off on Mike's PC.** Paths under `.platformio` get close to 260 characters.
- **Native tests on Windows need a host `g++`** (MSYS2 or WinLibs). Mike may not have one, so CI is the backstop.
- **PlatformIO converts `stopwatch.ino` to a temporary `stopwatch.ino.cpp`.** That's why `build_src_filter` says `+<*.ino.cpp>`.
- **ESP-IDF flags downgrade some warnings with `-Wno-error=...`.** `platformio.ini` re-enables them for our sources.
- **AI sandbox sessions:**
  - Git in a mounted folder may leave `.git/index.lock` behind if file deletion isn't permitted. Remove the lock before finishing.
  - Build in a copy outside the repo, so Linux `.pio/` output doesn't land in Mike's Windows checkout.

## 6. Hardware facts that bite

The full list is in `docs/HARDWARE.md`. These are the ones that cause bugs:

- **Buttons:** yellow = **G2** (BtnA), blue = **G1** (BtnB). Both are active-low with external pull-ups (verified in M5Unified source). The power button belongs to the PMIC (reset / power-off / download mode). It is not an app input.
- **Call `M5.begin()` before any `M5.Display` call.** Call `M5.update()` every loop.
- **`M5.Power.isCharging()` returns an enum, not a bool.** Battery current isn't readable. `getVBUSVoltage()` returns mV, or -1.
- **`M5.Power.setVibration(n)` runs until `setVibration(0)`.** Each call is an I²C write.
- **OPI PSRAM must be enabled** (`qio_opi`), or M5GFX mis-renders the AMOLED.
- **Touch:** don't use M5Unified's `wasClicked()` for taps. Its 8 px flick limit drops taps near the round edge. `Hardware::pollTouch` reports down / drag / tap, with a 20 px slop and the tap placed at the landing point. Give every new touch target a padded hit zone in `ui/MenuLayout.h`, and extend the overlap tests.
- **Text placement:** LovyanGFX datums align a font's *line box* (its tallest glyph to its lowest descender), not the visible caps. Don't eyeball Y positions on the round screen. Use the measured ink extents (`scripts/fontmetrics.py`) and the `ui/Layout.h` approach. That's how "DRAW" ended up touching the big number before.
- **Light sleep:**
  - GPIO wake reprograms the pin's interrupt type, so detach the timing ISR first and re-attach it after.
  - The wake press must be swallowed with `ButtonLogic::suppressUntilRelease()`.
- **Deep sleep:**
  - Wake is EXT1 any-low on G1/G2, and it reboots the device.
  - Call `rtc_gpio_deinit()` on those pins after waking.
  - History survives only through the RTC-memory snapshot (`hal/SleepMemory`).
- **When unsure how M5Unified or M5GFX behave on this board, read the library source** (`.pio/libdeps/stopwatch/...`; look for `board_M5StopWatch`) instead of guessing from docs. The docs have been wrong or ambiguous before. For example, the IO-expander pin numbers for the display rail disagree.

## 7. Working with Mike

- **Lead with the result.** Be concise, and skip the narration. When he asks for an explanation, give a complete one.
- **Explain trade-offs** for design choices in plain terms, and record them in the decision log.
- **Ask before:**
  - large refactors
  - changing pinned versions
  - deleting anything that isn't a build artifact
  - changing timing semantics (what a tap means)
- **Curling domain:**
  - Split times map to weight. The bands in `Config.h` are **placeholders** until Mike calibrates them.
  - Hog-to-hog of a draw is the "ice speed".
  - Hand-timing error is about ±0.05 s per tap, so the device must add far less than that.
