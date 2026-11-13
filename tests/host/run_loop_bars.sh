#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# engine.c loop_bars against 44.1 kHz and 22.05 kHz loops (tests/host/loop_bars_test.c). The function
# is cut out of firmware/src/app/engine.c, so the test runs the firmware's own code.
set -e
cd "$(dirname "$0")/../.."
CC="${CC:-cc}"
OUT=build/host
mkdir -p "$OUT"
# Match optional X0X_COLD (and similar) attrs before static — size lane cold-marks broke ^static.
awk '/^(X0X_[A-Z0-9_]+[[:space:]]+)*static uint8_t loop_bars\(/ {on = 1} on {print} on && /^}/ {exit}' firmware/src/app/engine.c > "$OUT/loop_bars.inc"
grep -q "loop_bars" "$OUT/loop_bars.inc" || { echo "loop_bars not found in engine.c"; exit 2; }
"$CC" -std=c99 -O2 -Wall -Wextra -Werror "-DLOOP_BARS_SRC=\"$PWD/$OUT/loop_bars.inc\"" -o "$OUT/loop_bars_test" tests/host/loop_bars_test.c
"$OUT/loop_bars_test"
