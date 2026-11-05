#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Actual-source held-note/manual/autosave regression; SAN_FLAGS enables sanitizers.
set -eu
cd "$(dirname "$0")/../.."
. tests/host/original_build.sh
OUT=${SAVE_NOTE_OUT:-build/host/save-note-quiet}
mkdir -p "$OUT"
sh host/build_host.sh --generate-only >/dev/null
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c firmware/src/app/factory_banks.c"
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter ${SAN_FLAGS:-} -DX0X_HOST -DBOARD_SMK37=1 $ORIGINAL_FLAGS -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    tests/host/save_note_quiet_test.c $ORIGINAL_SOURCES -lm -o "$OUT/test"
"$OUT/test"
