#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Matrix captions: mapped knob indices, sparse rows and clear navigation.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/matrix-knob-caption}
mkdir -p "$OUT"
awk '/^static void smk_wheels\(void\)/{on=1} on{print} on && /^}/{exit}' firmware/src/app/main_fm1.c > "$OUT/smk_wheels.inc"
test -s "$OUT/smk_wheels.inc"
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c firmware/src/app/factory_banks.c"
# shellcheck disable=SC2086
${CC:-cc} -O0 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter ${SAN_FLAGS:-} \
 -DX0X_HOST -DX0X_BASS_PUBLISH_TEST=1 -DX0X_SMK_CAPTIONS=${X0X_TEST_SMK_CAPTIONS:-1} -DX0X_WOBBLE=1 -DX0X_DEV_WOBBLE=1 -DX0X_BASS_PRESET_PREVIEW=1 -DX0X_BASS_TAGGED_SOUND=1 \
 -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" tests/host/matrix_knob_caption_test.c $U -lm -o "$OUT/test"
"$OUT/test" "$OUT"
