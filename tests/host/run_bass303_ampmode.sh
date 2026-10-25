#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host proof for Chicago AmpMod Env/Gate on native303. No schwung-303, no AU, no flash.
set -e
cd "$(dirname "$0")/../.."
OUT=build/bass303_ampmode
mkdir -p "$OUT"
CC="${CC:-cc}"
CFLAGS="-std=c99 -O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion"
$CC $CFLAGS -I firmware/src -I firmware/src/dsp \
  tests/host/bass303_ampmode_test.c firmware/src/dsp/bass303.c -lm -o "$OUT/ampmode"
"$OUT/ampmode"
