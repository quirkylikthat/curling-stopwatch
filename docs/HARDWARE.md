# Hardware notes: M5Stack StopWatch (C152)

Collected from the M5Stack docs and the M5Unified source (Oct 2026). Items marked ⚠ are unverified on a real device. Confirm them on first flash (see ARCHITECTURE.md §4).

## Specs

| | |
|---|---|
| SoC | ESP32-S3R8, dual-core 240 MHz, 16 MB flash, 8 MB OPI PSRAM |
| Display | 1.75" round AMOLED, 466×466, CO5300 over QSPI (`M5.Display`) |
| Touch | CST820B (`M5.Touch`), INT on G13 |
| Buttons | KEYA **yellow = G2**, KEYB **blue = G1**, plus a power button |
| Haptics | Vibration motor via M5IOE1 PWM: `M5.Power.setVibration(0–255)`, where 0 = off |
| Audio | ES8311 codec + AW8737A amp + 1 W speaker (`M5.Speaker`), MEMS mic |
| IMU / RTC | BMI270 @0x68 (`M5.Imu`), RX8130CE @0x32 (`M5.Rtc`) |
| Power | M5PM1 PMIC, 450 mAh battery. Charge current 185 mA by default |
| Internal I²C | SDA G47 / SCL G48. M5IOE1 @0x4F, ES8311 @0x18 |
| Grove (Port A) | G10 / G11 |

## Software versions

- Board package **M5Stack ≥ 3.3.7** (3.3.9 latest). Board name: **M5StopWatch**. Index URL: `https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json`
- **M5Unified ≥ 0.2.25** (StopWatch support from 0.2.16; ES8311 audio fix in 0.2.22; fix for `Display.init()` called before `begin()` in 0.2.25)
- **M5GFX ≥ 0.2.31** (required by M5Unified 0.2.24+)
- M5Unified board enum: `m5::board_t::board_M5StopWatch`. The board is detected **at runtime** by probing I²C devices; there is no compile-time board define.
- PlatformIO: pioarduino platform `55.03.39` (= Arduino-ESP32 3.3.9). Board JSON in `boards/`, pin variant in `variants/` (upstream has neither yet).
- **OPI PSRAM must be enabled** (`qio_opi`). M5GFX checks `CONFIG_SPIRAM_MODE_OCT` at compile time, and without it the AMOLED falls back to direct drawing, which can mis-render glyphs at odd coordinates.

## API gotchas

- **Call `M5.update()` every loop**, and keep the loop non-blocking.
- **Call `M5.begin()` before any `M5.Display` call.**
- **`M5.Power.isCharging()` returns an enum** (`is_discharging`, `is_charging`, `charge_unknown`), not a bool. Compare against `m5::Power_Class::is_charging`.
- **Battery current can't be read on this board.** Level (0–100) and voltage (mV) work.
- **`setVibration(n)` keeps running until you call `setVibration(0)`.** Every call is an I²C write to the IO expander, so don't spam it (Haptics de-duplicates writes).
- **Toggling display power or reset through M5IOE1 breaks the display.** M5Unified can't re-initialise the display on its own yet.
- **Power button** (⚠ per the docs):
  - Single click = power on / reset.
  - Double click = power off.
  - Hold ~2 s until the green LED lights = download mode.
  - Treat it as unavailable for app input.
- **Hardware v1.0 sticker error.** On some v1.0 units, the rear pin labelled `BAT` is actually **5V IN**. Never connect a battery to it. This is fixed in v1.0.1 (`*BAT`).

## Assumptions in this firmware

Verified by reading the M5Unified 0.2.25 / M5GFX 0.2.32 source and by compiling against board package 3.3.9:

| Fact | Source |
|---|---|
| BtnA = **G2**, BtnB = **G1**, both **active-low**, set up as plain `input` (external pull-ups) | `M5Unified.inl`, StopWatch case |
| M5Unified only *polls* G1/G2 (`gpio_in`), so our `attachInterrupt` on G2 doesn't conflict | same |
| `M5.Display.setBrightness()` works on the AMOLED (CO5300 → `Panel_AMOLED::setBrightness`, cmd 0x51) | M5GFX |
| FQBN is `m5stack:esp32:m5stack_stopwatch` | `arduino-cli board listall` |

Still ⚠ to confirm on a device (see ARCHITECTURE.md §4):

| Assumption | Where | If wrong |
|---|---|---|
| Interrupt fires reliably on the press edge | `Hardware.cpp` | Set `kUseInterruptTiming = false` |
| 425 KB PSRAM canvas allocates and pushes at ≥ 10 fps | `Renderer::begin` | Shrink the canvas to the number area |
| Layout fits inside the circle | `ui/Theme.h` | Adjust the Y positions / fonts |

## Sleep and wake-up

**What this firmware uses** (see ARCHITECTURE D17):
- **Light sleep:** `gpio_wakeup_enable()` on G1/G2 (level), plus `esp_sleep_enable_timer_wakeup()`. Our edge interrupt on G2 must be detached first, because GPIO wake reprograms the pin's interrupt type.
- **Deep sleep:** `esp_sleep_enable_ext1_wakeup_io(G1|G2, ESP_EXT1_WAKEUP_ANY_LOW)`. Both are RTC GPIOs (0–21 on the S3), and the external button pull-ups stay powered. After an EXT1 wake the pins are still in RTC mode, so `rtc_gpio_deinit()` hands them back to normal GPIO.
- **Display:** `M5.Display.sleep()` / `wakeup()` = brightness 0 + CO5300 sleep-in (0x10) / sleep-out (0x11).
- **USB power detection:** `M5.Power.getVBUSVoltage()` returns mV from the M5PM1 VIN register, or -1.
- **Status LED:** the board's **only** LED is `LED1`, which is **green-only**. It's driven by the M5PM1's `LED_EN` pin (schematic net `PM_STATUS_LED`), so it can be switched on or off, but not recoloured.
  - Firmware control: `M5.Power.M5pm1.setLedEnLevel(bool)`, which sets PWR_CFG (reg 0x06) bit 4.
  - M5GFX switches it **on** during board detection at every boot.
  - Because the M5PM1 is always powered, the LED keeps its state through ESP32 deep sleep.
  - The M5PM1 also lights it itself for download mode.
- **RTC memory:** `RTC_DATA_ATTR` puts the history snapshot (~1 KB) in RTC slow memory, which deep sleep retains.

**Other sources** (not used yet):
- The IMU INT1 and RTC interrupt both go to M5PM1 pin PYG0. M5PM1's IRQ output (PYG1) is wired to **ESP32 GPIO 12**.
- The M5PM1 library (`M5PM1.h`) offers `timerSet(seconds, action)`, `shutdown()`, `gpioSetWakeEnable()`, and `setLdoEnable()` + `ldoSetPowerHold()` to keep the IMU/RTC powered while asleep.
- Reference: <https://docs.m5stack.com/en/arduino/stopwatch/m5pm1_m5ioe1>

## Links

- Product page: <https://docs.m5stack.com/en/core/StopWatch>
- Arduino quick start: <https://docs.m5stack.com/en/arduino/stopwatch/program>
- Schematic: on the product page
- Factory firmware (ESP-IDF, useful UI reference): <https://github.com/m5stack/M5StopWatch-UserDemo>
