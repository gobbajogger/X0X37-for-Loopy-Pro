#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Exercise production UI/engine/DSP; no target build or physical device.
set -eu
TEST_ROOT=$(CDPATH= cd "$(dirname "$0")/../.." && pwd)
SRC=${X0X_SOURCE_ROOT:-$TEST_ROOT}
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
cd "$SRC"
sh host/build_host.sh --generate-only > "$OUT/build.log" 2>&1 || { cat "$OUT/build.log"; exit 1; }
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c
firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c
firmware/src/seq/pattern.c firmware/src/seq/motion.c"
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -D_POSIX_C_SOURCE=200809L -DX0X_HOST ${X0X_HOST_DEFS:-} ${SAN_FLAGS:-} \
    -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
    "-DX0X_HOST_SOURCE=\"$SRC/host/x0x_host.c\"" \
    "-DX0X_ENGINE_SOURCE=\"$SRC/firmware/src/app/engine.c\"" \
    "$TEST_ROOT/tests/host/motion_ui_test.c" $U -lm -o "$OUT/motion_ui_test"
"$OUT/motion_ui_test"
