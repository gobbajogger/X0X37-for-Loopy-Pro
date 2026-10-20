#!/bin/sh
set -e
cd "$(dirname "$0")/../.."
OUT=${OUT:-build/host}
mkdir -p "$OUT"
${CC:-cc} -O2 -ffp-contract=off -std=c99 -w -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    -o "$OUT/song_actions_test" tests/host/song_actions_test.c firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c \
    firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
    firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c \
    firmware/src/app/engine.c -lm
"$OUT/song_actions_test"
