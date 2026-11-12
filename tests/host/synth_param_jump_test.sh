#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# B4 recurrent ADC outliers against the real scanner, original engines and lanes.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/synth-param-jump}
mkdir -p "$OUT"
awk '/^static void smk_wheels\(void\)/{on=1} on{print} on && /^}/{exit}' firmware/src/app/main_fm1.c > "$OUT/smk_wheels.inc"
test -s "$OUT/smk_wheels.inc"
awk '/^static void smk__wheel\(/{on=1} on{print} on && /^}/{exit}' firmware/hal/smk_input.h > "$OUT/smk_wheel_adc.inc"
test -s "$OUT/smk_wheel_adc.inc"
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1
. tests/host/original_build.sh
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter ${SAN_FLAGS:-} \
 -DX0X_HOST -DX0X_BASS_PUBLISH_TEST=1 -DX0X_SMK_CAPTIONS=1 -DBLEC_TWO_PEER=1 \
 -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" \
 tests/host/synth_param_jump_test.c $ORIGINAL_FLAGS $ORIGINAL_SOURCES -lm -o "$OUT/test"
for voice in 0 1; do
    for bank in 0 1; do
        for record in 0 1; do "$OUT/test" "$voice" "$bank" "$record" 0; done
        "$OUT/test" "$voice" "$bank" 0 1
        "$OUT/test" "$voice" "$bank" 0 2
        "$OUT/test" "$voice" "$bank" 0 3
    done
done
