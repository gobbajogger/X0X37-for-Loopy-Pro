#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
REF=${1:?sanitized selector-witness.bin};EDGE=${2:?selector-edge-witness.bin};OUT=${3:?output directory}
mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
for mode in normal sanitized;do
 flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -Iexperiments/original_synth tests/host/original_chicago_selector_test.c experiments/original_synth/chicago_selector.c -o "$OUT/selector-$mode"
 "$OUT/selector-$mode" "$REF" "$EDGE" > "$OUT/selector-$mode.log"
done
cmp "$OUT/selector-normal.log" "$OUT/selector-sanitized.log"
printf 'PASS original Chicago selector/gate producer checks; no external review\n'
