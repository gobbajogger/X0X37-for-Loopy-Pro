#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Exact silence skip of restored Chicago inserts: bytes and samples identical.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/chicago_fx2_zero_skip}; mkdir -p "$OUT"
for flags in '-O2 -ffp-contract=off -fno-fast-math' '-O1 -g -ffp-contract=off -fsanitize=address,undefined -fno-sanitize-recover=all'; do
    ${CC:-cc} -std=c99 $flags -Wall -Wextra -Ifirmware/src -Ifirmware/src/app \
        tests/host/chicago_fx2_zero_skip_test.c -lm -o "$OUT/t"
    "$OUT/t" "${SEEDS:-40}"
done
