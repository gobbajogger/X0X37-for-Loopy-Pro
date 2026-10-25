#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host proof for Chicago amp Env Attack/Decay/Sustain/Release on native303.
set -e
cd "$(dirname "$0")/../.."
OUT=build/bass303_amp_adsr
mkdir -p "$OUT"
CC="${CC:-cc}"
CFLAGS="-std=c99 -O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion"
$CC $CFLAGS -I firmware/src -I firmware/src/dsp \
  tests/host/bass303_amp_adsr_test.c firmware/src/dsp/bass303.c -lm -o "$OUT/amp_adsr"
"$OUT/amp_adsr"
