#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Compare settled specialization and tail fallback against ordinary processing.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-/tmp/chicago_insert_idle_identity}; mkdir -p "$OUT"
for flags in '-O2' '-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'; do
    ${CC:-cc} -std=c99 $flags -Wall -Wextra -fno-fast-math -ffp-contract=off -DORIGINAL_USE_SOFT_FMA tests/host/chicago_insert_idle_identity_test.c experiments/original_synth/chicago_program.c experiments/original_synth/fused_reference.c -lm -o "$OUT/test"
    "$OUT/test"
done
