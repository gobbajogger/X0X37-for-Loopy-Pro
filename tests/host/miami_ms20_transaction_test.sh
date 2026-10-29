#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Exact candidate/reference transaction comparison; separate immutable source.
# Optional second argument --bench runs alternating host CPU measurements.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
BASE=${MIAMI_MS20_REFERENCE_REF:-348ae21514e4688874d7e32ececd97fece6a018b}
PATH_IN_TREE=experiments/original_synth/miami_ms20.c
git rev-parse "$BASE:$PATH_IN_TREE" > "$OUT/reference-blob.txt"
git show "$BASE:$PATH_IN_TREE" > "$OUT/reference.c"
COMMON='-std=c99 -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble'
RENAMES='-Doriginal_miami_nonlinear=reference_miami_nonlinear -Doriginal_miami_mg_shape=reference_miami_mg_shape -Doriginal_miami_ms20_init=reference_miami_ms20_init -Doriginal_miami_ms20_note_reset=reference_miami_ms20_note_reset -Doriginal_miami_ms20_prepare=reference_miami_ms20_prepare -Doriginal_miami_ms20_sample=reference_miami_ms20_sample'
for arithmetic in native soft;do
 for mode in normal sanitized;do
  flags=-O2
  [ "$mode" != sanitized ] || flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
  [ "$arithmetic" != soft ] || flags="$flags -DORIGINAL_USE_SOFT_FMA"
  stem="$OUT/$arithmetic-$mode"
  ${CC:-cc} $COMMON $flags $RENAMES -c "$OUT/reference.c" -o "$stem-reference.o"
  ${CC:-cc} $COMMON $flags -c "$PATH_IN_TREE" -o "$stem-candidate.o"
  ${CC:-cc} $COMMON $flags tests/host/miami_ms20_transaction_test.c \
   "$stem-reference.o" "$stem-candidate.o" \
   experiments/original_synth/fused_reference.c -lm -o "$stem"
  "$stem" > "$stem.log"
  cat "$stem.log"
 done
done
if [ "${2:-}" = --bench ];then
 "$OUT/native-normal" --bench > "$OUT/host-benchmark.log"
 cat "$OUT/host-benchmark.log"
fi
