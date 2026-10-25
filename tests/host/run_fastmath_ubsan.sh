#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Guard the exponent bit adjustment against negative signed-shift UB.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
for test in fastmath_test fastmath_exponent_test; do
"${CC:-cc}" -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror \
    -fsanitize=undefined -fno-sanitize-recover=undefined \
    "tests/host/$test.c" -lm -o "$OUT/$test"
"$OUT/$test"
done
