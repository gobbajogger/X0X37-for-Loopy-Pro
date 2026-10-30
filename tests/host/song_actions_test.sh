#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host unit for song arrange actions. Cold trees lack build/gen; generate first
# (same gate as ui_feedback / project_stage) so drum909_dsp.h finds tables.
set -e
cd "$(dirname "$0")/../.."
OUT=${OUT:-build/host}
mkdir -p "$OUT" build/gen
sh host/build_host.sh --generate-only >/dev/null
${CC:-cc} -O2 -ffp-contract=off -std=c99 -w -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    -o "$OUT/song_actions_test" tests/host/song_actions_test.c firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c \
    firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
    firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c \
    firmware/src/app/engine.c -lm
"$OUT/song_actions_test"
