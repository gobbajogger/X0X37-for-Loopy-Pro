#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host unit for song arrange actions. Cold trees lack build/gen; generate first
# (same gate as ui_feedback / project_stage) so drum909_dsp.h finds tables.
set -e
cd "$(dirname "$0")/../.."
. tests/host/original_build.sh
OUT=${OUT:-build/host}
mkdir -p "$OUT" build/gen
sh host/build_host.sh --generate-only >/dev/null
${CC:-cc} -O2 -ffp-contract=off -std=c99 -w -DX0X_HOST $ORIGINAL_FLAGS -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    tests/host/song_actions_test.c $ORIGINAL_SOURCES -lm -o "$OUT/song_actions_test"
"$OUT/song_actions_test"
