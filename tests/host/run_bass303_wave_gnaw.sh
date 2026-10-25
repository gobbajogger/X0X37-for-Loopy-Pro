#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host proof for Chicago continuous Wave morph + VCF Gnaw on native303.
set -e
cd "$(dirname "$0")/../.."
OUT=build/bass303_wave_gnaw
mkdir -p "$OUT"
CC="${CC:-cc}"
CFLAGS="-std=c99 -O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion"
$CC $CFLAGS -I firmware/src -I firmware/src/dsp \
  tests/host/bass303_wave_gnaw_test.c firmware/src/dsp/bass303.c -lm -o "$OUT/wave_gnaw"
"$OUT/wave_gnaw"
