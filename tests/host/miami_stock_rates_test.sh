#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Actual production stock decoder, engine and UI; synthetic preservation fixtures.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1 || { cat "$OUT/assets.log"; exit 1; }
FLAGS="-O1 -g -ffp-contract=off -std=c99 -DX0X_BASS_PRESET_PREVIEW=1 -DX0X_BASS_TAGGED_SOUND=1 -DX0X_BASS_NOTE_SOUND=0 -DX0X_HOST -D_DARWIN_C_SOURCE -D_POSIX_C_SOURCE=200809L -DX0X_WOBBLE=1 -DX0X_DEV_WOBBLE=1 -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen"
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c experiments/wobble/wobble.c"
${CC:-cc} $FLAGS ${SAN_FLAGS:-} -Dwobble_set_tempo=miami_observe_tempo -c firmware/src/app/engine.c -o "$OUT/engine.o"
${CC:-cc} $FLAGS ${SAN_FLAGS:-} -Wall -Wextra -Wno-unused-function -Wno-unused-parameter tests/host/miami_stock_rates_test.c $U "$OUT/engine.o" -lm -o "$OUT/probe"
"$OUT/probe" "$@"
