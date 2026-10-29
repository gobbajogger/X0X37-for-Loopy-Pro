#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
REF=${1:?Chicago inventory JSON};OUT=${2:?output directory}
mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
python3 tools/gen_chicago_program_private.py "$REF" "$OUT/chicago_program_private.h" "$OUT/saved139.bin" > "$OUT/generation.json"
cmp experiments/original_synth/chicago_program_private.h "$OUT/chicago_program_private.h"
python3 tools/gen_chicago_program_private.py "$REF" "$OUT/chicago_program_expanded.h" "$OUT/saved139-expanded.bin" --expanded > "$OUT/generation-expanded.json"
cmp "$OUT/saved139.bin" "$OUT/saved139-expanded.bin"
python3 tests/host/original_chicago_program_vectors.py "$REF" "$OUT/edits.bin" > "$OUT/vector-generation.json"
for arithmetic in native soft;do
 extra='';[ "$arithmetic" != soft ]||extra='-DORIGINAL_USE_SOFT_FMA'
 for mode in normal sanitized;do
  flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
  exe="$OUT/program-$arithmetic-$mode"
  ${CC:-cc} -std=c99 $flags $extra -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth tests/host/original_chicago_program_test.c experiments/original_synth/chicago_program.c experiments/original_synth/fused_reference.c -lm -o "$exe"
  "$exe" "$OUT/saved139.bin" "$OUT/edits.bin" > "$exe.log"
 done
done
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -I"$OUT" '-DORIGINAL_CHICAGO_PROGRAM_METADATA="chicago_program_expanded.h"' tests/host/original_chicago_program_test.c experiments/original_synth/chicago_program.c -lm -o "$OUT/program-expanded"
"$OUT/program-expanded" "$OUT/saved139.bin" "$OUT/edits.bin" > "$OUT/program-expanded.log"
printf 'PASS Chicago original81contexts/all22controls/canonical117fields/routing, native/software FMA normal/ASan/UBSan\n'
