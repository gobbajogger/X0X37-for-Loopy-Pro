#!/bin/sh
# Original Miami numeric data/list/caller fixtures;no native or activation.
set -eu
cd "$(dirname "$0")/../.."
FIXTURE=${1:?original numeric fixture};OUT=${2:?fresh output directory};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
for mode in normal sanitized;do
 flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
 cc -std=c99 $flags -Wall -Wextra -Werror -pedantic -Iexperiments/original_synth tests/host/original_miami_geometry_test.c experiments/original_synth/miami_geometry.c experiments/original_synth/chicago_allocator.c -o "$OUT/$mode"
 "$OUT/$mode" "$FIXTURE" > "$OUT/$mode.log"
done
cmp "$OUT/normal.log" "$OUT/sanitized.log"
cat "$OUT/normal.log"
