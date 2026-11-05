#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Guard the exponent bit adjustment against negative signed-shift UB.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
"${CC:-cc}" -DX0X_HOST -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror \
    -fsanitize=undefined -fno-sanitize-recover=undefined \
    tests/host/fastmath_test.c -lm -o "$OUT/fastmath_ubsan"
"$OUT/fastmath_ubsan"
