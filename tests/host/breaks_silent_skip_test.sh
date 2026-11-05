#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/breaks_silent_skip}; mkdir -p "$OUT"
for flags in -O2 '-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'; do
  ${CC:-cc} -std=c99 $flags -Wall -Wextra -Ifirmware/src -Ifirmware/src/dsp \
    tests/host/breaks_silent_skip_test.c -lm -o "$OUT/t"
  "$OUT/t"
done
