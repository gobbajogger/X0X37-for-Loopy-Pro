#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory}; mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
for arithmetic in native soft; do
    for mode in normal sanitized; do
        flags=-O2
        [ "$mode" != sanitized ] || flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
        [ "$arithmetic" != soft ] || flags="$flags -DORIGINAL_USE_SOFT_FMA"
        exe="$OUT/cell-$arithmetic-$mode"
        ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math \
            -Iexperiments/original_synth tests/host/original_sound_cell_test.c firmware/src/app/original_sound.c \
            experiments/original_synth/miami_program.c experiments/original_synth/chicago_program.c \
            experiments/original_synth/fused_reference.c -lm -o "$exe"
        "$exe" > "$exe.log"
    done
done
for variant in native-sanitized soft-normal soft-sanitized; do
    cmp "$OUT/cell-native-normal.log" "$OUT/cell-$variant.log"
done
cat "$OUT/cell-native-normal.log"
