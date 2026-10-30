#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Exact IEEE-binary32 table interpolation, no external or private data.
set -eu
cd "$(dirname "$0")/../.."
OUT="${1:-build/host/miami-table-interpolation}"
mkdir -p "$OUT"
CC="${CC:-cc}"
for opt in Os O2; do
    "$CC" -std=c99 "-$opt" -Wall -Wextra -Werror -fno-fast-math \
        -ffp-contract=off -frounding-math -Iexperiments/original_synth \
        tests/host/miami_table_interpolation_test.c \
        experiments/original_synth/fused_reference.c -lm \
        -o "$OUT/table-$opt"
    "$OUT/table-$opt"
done
