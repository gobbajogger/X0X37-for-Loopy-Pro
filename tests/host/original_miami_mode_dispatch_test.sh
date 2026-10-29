#!/bin/sh
# Exact original Miami numeric fixtures at the C6A4 callback seam.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output};FIXTURE=${2:?original numeric fixture};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
for mode in normal sanitized;do
 flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
 cc -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -Iexperiments/original_synth tests/host/original_miami_mode_dispatch_test.c experiments/original_synth/chicago_allocator.c -lm -o "$OUT/$mode"
 "$OUT/$mode" "$FIXTURE" > "$OUT/$mode.log"
done
cmp "$OUT/normal.log" "$OUT/sanitized.log"
cat "$OUT/normal.log"
