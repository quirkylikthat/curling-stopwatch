# Project status

**Last updated:** 2026-10-10.

This is the source of truth for **what actually works**. Update it whenever something is confirmed on the device, or found broken. Agents must not mark anything ✅ on device without Mike's confirmation.

## Status key

- ✅ verified
- 🟡 compiles, but its behaviour isn't confirmed on the device
- ⬜ not applicable
- ❌ known broken

## Features

| Feature | Unit tests | Builds (both toolchains) | On device |
|---|---|---|---|
| Split / hog-to-hog / combined timing (`SequenceTimer`) | ✅ | ✅ | ✅ |
| Interrupt-timestamped yellow button | ⬜ | ✅ | ✅ |
| Blue click (next mode, or cancel a run) / hold (clear history) | ✅ (`ButtonLogic`) | ✅ | ✅ |
| Weight bands (split) | ✅ | ✅ | ✅ (bands are **placeholders**, not calibrated) |
| Screen layout on the round AMOLED, flicker-free canvas | ✅ (`ViewBuilder`) | ✅ | ✅ |
| Haptic feedback patterns | ⬜ | ✅ | ✅ |
| Battery arc + charging colour | ⬜ | ✅ | ✅ |
| Mode remembered across power cycles (NVS) | ⬜ | ✅ | ✅ |
| Serial CSV output | ⬜ | ✅ | 🟡 |
| Dim after 1 min idle | ✅ (`PowerPolicy`) | ✅ | ✅ |
| Light sleep after 10 min (instant wake, yellow wake press counts as a tap) | ✅ | ✅ | ✅ (bench-tested 2026-10-10) |
| Deep sleep after 30 min (wake on either button, history restored) | ✅ (`HistorySnapshot`) | ✅ | ✅ (bench-tested 2026-10-10) |
| Status LED: off while awake, **flash every 5 s in light sleep, off in deep sleep** (D20; replaces solid green) | ⬜ | ✅ | ✅ |
| History screen: last 10 throws, split + hog-to-hog columns (blue double-click) | ✅ (`ViewBuilder`) | ✅ | ✅ |
| Blue double-click detection (single click delayed by 0.3 s) | ✅ (`ButtonLogic`) | ✅ | ✅ |
| IMU held in suspend (`kImuEnabled = false`) | ⬜ | ✅ | ✅ |
| Grove 5 V booster off (`kGrovePortPower = false`) | ⬜ | ✅ | ✅ |
| 1 ms idle per loop (`kLoopIdleMs`) | ⬜ | ✅ | ✅ |
| Even spacing of the band label / secondary line (`ui/Layout.h`) | ✅ | ✅ | ✅ |
| Menu (2 s blue hold), touch navigation, BACK / blue click | ✅ (`Menu`) | ✅ | 🟡 |
| Settings → Brightness slider (live, saved) | ✅ (`UserSettings`, `Menu`) | ✅ | 🟡 |
| Settings → Split bands (−/+, SHIFT ALL, RESET; saved; used for labels/colours) | ✅ | ✅ | 🟡 |
| Clear history moved to the menu, with confirmation | ✅ | ✅ | 🟡 |
| Export | ⬜ | ⬜ | ⬜ (placeholder page) |

## Toolchains

| Check | Status |
|---|---|
| Arduino CLI build: m5stack:esp32 3.3.9, M5Unified 0.2.25, M5GFX 0.2.32 | ✅ in the AI sandbox and in CI (`scripts/verify.sh`) |
| PlatformIO build: pioarduino 55.03.39 | ✅ in the sandbox. ✅ on Mike's PC (compiled and linked; see the 2026-10-10 log) |
| Host unit tests (70) | ✅ sandbox. On Mike's PC, needs a `g++` |
| GitHub Actions CI | 🟡 Workflow written but not yet run; the repo isn't pushed to GitHub yet |

## Known issues

- **pioarduino 55.03.39 crashes on `pio run -v`** (verbose builds). Normal builds are fine.
- **Sleep and awake current haven't been measured.** The display rail and touch controller stay powered in deep sleep (ARCHITECTURE D17), and the D20 savings are estimates.
- **The default weight bands are uncalibrated placeholders.** They can now be tuned on the device (Menu → Settings → Split bands).

## Next steps

1. **Flash and run** ARCHITECTURE §4 checklist items 1–7. Update the table above.
2. **Push to GitHub** so CI runs on every change.
3. **Calibrate the split bands** on real ice.
4. **Pick the next feature** from the ARCHITECTURE §5 roadmap.
