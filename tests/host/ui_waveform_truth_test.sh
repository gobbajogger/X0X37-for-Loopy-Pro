#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Miami TRI/SQR/PUL value-cell shapes sample the real periodic function.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/host/ui-waveform-truth}
mkdir -p "$OUT"
${CC:-cc} -O1 -ffp-contract=off -std=c99 -Wno-attributes ${SAN_FLAGS:-} tests/host/ui_waveform_truth_test.c -lm -o "$OUT/truth"
"$OUT/truth"
