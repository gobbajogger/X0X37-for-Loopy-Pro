#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Run the existing recording regression against the supported full app graph.
set -eu
cd "$(dirname "$0")/../.."
. tests/host/original_build.sh
sh host/build_host.sh --generate-only >/dev/null
BOARD_FLAG=-DBOARD_SMK37=1   # SMK-37 only
OUT=build/host/rec_live_smk37_test
${CC:-cc} -O2 -std=c99 -w -DX0X_HOST $BOARD_FLAG \
    -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen $ORIGINAL_FLAGS \
    tests/host/rec_live_test.c $ORIGINAL_SOURCES -lm -o "$OUT"
"$OUT"
