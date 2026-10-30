#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Actual Chicago control/tempo/tail regressions in both engine configurations.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1 || { cat "$OUT/assets.log"; exit 1; }
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/factory_banks.c experiments/wobble/wobble.c"
for WBL in 0 1; do
    ${CC:-cc} -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter ${SAN_FLAGS:-} \
        -DX0X_HOST -DX0X_WOBBLE=$WBL -DX0X_DEV_WOBBLE=$WBL \
        -DX0X_BASS_PRESET_PREVIEW=1 -DX0X_BASS_TAGGED_SOUND=1 -DX0X_BASS_NOTE_SOUND=0 \
        -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
        tests/host/chicago_fx_runtime_test.c tests/host/chicago_fx_runtime_engine.c $U -lm \
        -o "$OUT/chicago-$WBL" >"$OUT/compile-$WBL.log" 2>&1 || { cat "$OUT/compile-$WBL.log"; exit 1; }
    X0X_HOST_LOAD=0 "$OUT/chicago-$WBL"
done
