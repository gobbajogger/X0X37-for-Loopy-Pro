#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Producer proof of finite widening and unchanged double-order converter.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
for mode in normal sanitized;do
 flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
 exe="$OUT/widen-$mode"
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -Iexperiments/original_synth tests/host/original_binary32_widen_test.c experiments/original_synth/chicago_tuning.c experiments/original_synth/fused_reference.c -DORIGINAL_USE_SOFT_FMA -lm -o "$exe"
 "$exe" > "$exe.log"
done
cmp "$OUT/widen-normal.log" "$OUT/widen-sanitized.log"
cat "$OUT/widen-normal.log"
