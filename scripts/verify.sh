#!/usr/bin/env bash
# verify.sh -- the single "is this change OK?" gate. CI runs exactly this.
#
#   scripts/verify.sh            everything that's installed
#   scripts/verify.sh --quick    format + unit tests only (seconds)
#
# Checks, in order (each is skipped with a notice if its tool isn't installed,
# except unit tests, which always run):
#   1. clang-format         no formatting drift            (pinned: 23.x)
#   2. unit tests           make -C test (g++, -Werror)
#   3. PlatformIO firmware  pio run -e stopwatch           (-Werror on our sources)
#   4. Arduino firmware     arduino-cli compile, fails on any warning in our sources
set -euo pipefail
cd "$(dirname "$0")/.."

QUICK=0
[[ "${1:-}" == "--quick" ]] && QUICK=1
FQBN="m5stack:esp32:m5stack_stopwatch"
have() { command -v "$1" >/dev/null 2>&1; }
step() { printf '\n=== %s ===\n' "$1"; }

step "1/4 format"
if have clang-format; then
    git ls-files 'src/*.h' 'src/*.cpp' 'test/*.h' 'test/*.cpp' | xargs clang-format --dry-run --Werror
    echo "ok"
else
    echo "skipped (clang-format not installed: pip install clang-format==23.1.3)"
fi

step "2/4 unit tests"
make -C test --no-print-directory
if have python3; then
    PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s scripts
fi

if [[ $QUICK == 1 ]]; then
    echo; echo "quick checks passed"; exit 0
fi

step "3/4 firmware (PlatformIO)"
if have pio; then
    pio run -e stopwatch
else
    echo "skipped (PlatformIO not installed)"
fi

step "4/4 firmware (Arduino CLI)"
if have arduino-cli; then
    # Arduino requires the sketch folder name to match stopwatch.ino.
    tmp="$(mktemp -d)"
    mkdir -p "$tmp/stopwatch"
    tar --exclude=.pio --exclude=.git -cf - . | tar -xf - -C "$tmp/stopwatch"
    log="$tmp/arduino.log"
    arduino-cli compile --fqbn "$FQBN" --warnings all "$tmp/stopwatch" 2>&1 | tee "$log"
    if grep -E "stopwatch/(src/|stopwatch\.ino).*(warning|error):" "$log"; then
        echo "FAIL: warnings in our sources (see above)"; exit 1
    fi
    rm -rf "$tmp"
else
    echo "skipped (arduino-cli not installed)"
fi

echo; echo "all checks passed"
