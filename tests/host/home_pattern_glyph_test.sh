#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${HOME_GLYPH_OUT:-build/host/home-pattern-glyph}
mkdir -p "$OUT"
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Werror -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/seq \
    tests/host/home_pattern_glyph_test.c -o "$OUT/test"
"$OUT/test"
