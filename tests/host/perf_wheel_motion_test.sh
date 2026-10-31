#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Pitch bend and damper automation from the device wheels and external MIDI.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/perf-wheel-motion}
mkdir -p "$OUT"
awk '/^static void smk_wheels\(void\)/{on=1} on{print} on && /^}/{exit}' firmware/src/app/main_fm1.c > "$OUT/smk_wheels.inc"
test -s "$OUT/smk_wheels.inc"
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1
. tests/host/original_build.sh
U=$ORIGINAL_SOURCES
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter ${SAN_FLAGS:-} \
 -DX0X_HOST -DX0X_BASS_PUBLISH_TEST=1 -DX0X_SMK_CAPTIONS=1   -DX0X_BASS_PRESET_PREVIEW=1 -DX0X_BASS_TAGGED_SOUND=1 \
 -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" tests/host/perf_wheel_motion_test.c $ORIGINAL_FLAGS $U -lm -o "$OUT/test"
"$OUT/test"
