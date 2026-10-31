#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/host/ui-depth-wave}
mkdir -p "$OUT"
${CC:-cc} -O1 -ffp-contract=off -std=c99 -Wno-attributes ${SAN_FLAGS:-} tests/host/ui_waveform_truth_test.c -lm -o "$OUT/truth"
"$OUT/truth"
NIGHT_UI_TEST_FILE=tests/host/ui_depth_wave_test.c sh tests/host/night_ui_test.sh "$OUT"
