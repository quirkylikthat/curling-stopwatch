# Pinned versions: register and monthly review

Everything that is pinned lives here, with where it is set and where to check for updates. This file is how the monthly review stays quick and checkable.

**Last reviewed: 2026-10-11.** **Next review due: by 2026-11-15.**

## Why review monthly

Pins make builds reproducible, but they also stop us noticing when something is deprecated, end-of-life or insecure. The Node 20 removal and the `ubuntu-latest` move to Ubuntu 26.04 were both announced months ahead and only surfaced as run warnings. A monthly look costs about 15 minutes and catches that kind of thing early.

**Review is not upgrade.** Check each item for security fixes, end-of-life and deprecation notices, and breaking changes coming. Upgrade only when there's a reason, or when it's a CI-only item that CI will verify. Never upgrade firmware libraries without asking Mike (AGENTS.md §7): they need an on-device re-test.

## Register

"Latest seen" is what the upstream project listed on the review date.

### Firmware build (changing any of these needs a re-test on the device)

| Item | Pinned to | Latest seen (2026-10-11) | Set in | Check for updates |
|---|---|---|---|---|
| pioarduino platform (= Arduino-ESP32 core) | 55.03.39 (3.3.9) | **55.03.312-1 (3.3.12, IDF 5.5.5)**: behind, see log | `platformio.ini` | [pioarduino releases](https://github.com/pioarduino/platform-espressif32/releases), [Arduino-ESP32 releases](https://github.com/espressif/arduino-esp32/releases) |
| M5Stack board package (Arduino IDE / arduino-cli) | 3.3.9 | Not confirmed (StopWatch needs 3.3.7 or newer) | `ci.yml`, `docs/DEVELOPMENT.md` | Arduino IDE *Boards Manager*, or `arduino-cli core search m5stack`; index: [package_m5stack_index.json](https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json) |
| M5Unified | 0.2.25 | 0.2.25 (current) | `platformio.ini`, `ci.yml`, `docs/DEVELOPMENT.md` | [M5Unified releases](https://github.com/m5stack/M5Unified/releases) |
| M5GFX | 0.2.32 | 0.2.32 (current) | `platformio.ini`, `ci.yml`, `docs/DEVELOPMENT.md` | [M5GFX releases](https://github.com/m5stack/M5GFX/releases) |

Keep the board package and pioarduino on the same Arduino-ESP32 version.

### CI and release tooling (CI verifies these)

| Item | Pinned to | Latest seen (2026-10-11) | Set in | Check for updates |
|---|---|---|---|---|
| clang-format | 23.1.3 | 23.1.3 (current) | `ci.yml`, `AGENTS.md` | [PyPI](https://pypi.org/project/clang-format/) |
| PlatformIO Core | 6.2.0 | 6.2.0 (current) | `ci.yml`, `release.yml`, `AGENTS.md` | [PyPI](https://pypi.org/project/platformio/) |
| arduino-cli | 1.5.1 | 1.5.1 (current) | `ci.yml`, `AGENTS.md` | [releases](https://github.com/arduino/arduino-cli/releases) |
| Python (CI) | 3.12 | Supported until Oct 2028 | `ci.yml`, `release.yml` | [Python versions](https://devguide.python.org/versions/) |
| Runner image | `ubuntu-24.04` | `ubuntu-latest` moves to 26.04 from 19 Oct 2026 | `ci.yml`, `release.yml` | [runner-images announcements](https://github.com/actions/runner-images/issues); test `ubuntu-26.04` on purpose |
| actions/checkout | v7 | v7.0.1 | `ci.yml`, `release.yml` | Dependabot |
| actions/setup-python | v7 | v7.0.0 | `ci.yml`, `release.yml` | Dependabot |
| actions/cache | v5 | v5 line (Node 24) | `ci.yml`, `release.yml` | Dependabot |
| actions/upload-artifact | v7 | v7.0.2 | `release.yml` | Dependabot |
| arduino/setup-arduino-cli | v2 (still Node 20 upstream) | v2.0.0 | `ci.yml` | Dependabot |

Also read the annotations on the latest CI and release runs. Deprecation notices show up there first.

## Dependabot

`.github/dependabot.yml` checks the GitHub Actions versions once a month and opens **one grouped pull request** if anything has a newer release. It runs CI on that PR, so a green check is the verification. It does not cover the pip pins (clang-format, PlatformIO), the runner image, Python, or any firmware library: those stay in the manual review.

When a Dependabot PR arrives: read the release notes for breaking changes, merge if CI is green, then update the "Pinned to" cells above and the review log.

## How to run the review

1. Walk the tables above and note the latest version of each item (and any deprecation or end-of-life notice). Update "Latest seen".
2. Read the [GitHub Actions changelog](https://github.blog/changelog/label/actions/) for anything that affects runners, Node, or the actions we use. Merge or close any open Dependabot PR.
3. For each item decide: **no action**, **upgrade now** (CI-only, with a reason), or **schedule** (firmware libraries: batch them, ask Mike, and re-test on the device).
4. If you change a pin, change it everywhere listed in "Set in", update this file, and add a CHANGELOG line.
5. Update **Last reviewed** and **Next review due** at the top, and add a row to the log below.

## Review log

| Date | By | Outcome |
|---|---|---|
| 2026-10-11 | Claude, with Mike | **First full review.** Current: M5Unified 0.2.25, M5GFX 0.2.32, clang-format 23.1.3, PlatformIO 6.2.0, arduino-cli 1.5.1, Python 3.12. CI actions moved to Node 24 versions; runner pinned to `ubuntu-24.04` ahead of the `ubuntu-latest` move on 19 Oct 2026; Dependabot added for actions. `setup-arduino-cli` has no Node 24 release yet. **Open items:** (1) **pioarduino is three patch releases behind** (55.03.39 / Arduino-ESP32 3.3.9 vs 55.03.312-1 / 3.3.12, IDF 5.5.5). Nothing urgent for this firmware (the 3.3.x fixes we saw are in the web server and BLE, which we don't use). **Worth trying:** the newest release's notes describe a fix for a verbose-compile crash that may be our known `pio run -v` failure (unconfirmed). Schedule as one batch with the M5Stack board package, build in CI, and re-test on the device before adopting. (2) The M5Stack Arduino board package's latest version could not be confirmed from here: check it in the Arduino IDE *Boards Manager*. |
