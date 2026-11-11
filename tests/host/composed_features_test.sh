#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${COMPOSED_FEATURE_OUT:-build/host/composed-features}
mkdir -p "$OUT"
sh host/build_host.sh --generate-only > "$OUT/generate.log" 2>&1
. tests/host/original_build.sh
EPOCH=
grep -q 'static void host_midi_put' host/x0x_host.c && EPOCH=-DTEST_MIDI_EPOCH=1
${CC:-cc} -O1 -g -std=c99 -ffp-contract=off -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
 -DX0X_HOST -D_POSIX_C_SOURCE=200809L $EPOCH ${SAN_FLAGS:-} \
 -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
 tests/host/composed_features_test.c $ORIGINAL_FLAGS $ORIGINAL_SOURCES -lm -o "$OUT/test" > "$OUT/compile.log" 2>&1
"$OUT/test"
