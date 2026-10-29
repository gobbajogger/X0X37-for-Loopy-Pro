#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host unit for engine/DSP copy semantics. Cold trees lack build/gen; generate
# first (same gate as song_actions / ui_feedback) so drum909_dsp.h finds tables.
set -e
cd "$(dirname "$0")/../.."
. tests/host/original_build.sh
OUT=${OUT:-build/host}
mkdir -p "$OUT" build/gen
sh host/build_host.sh --generate-only >/dev/null
${CC:-cc} -O2 -ffp-contract=off -std=c99 -w -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    -o "$OUT/copy_semantics_test" tests/host/copy_semantics_test.c    \
    $ORIGINAL_FLAGS $ORIGINAL_SOURCES    \
     -lm
"$OUT/copy_semantics_test"
