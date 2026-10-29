#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Cohesive inactive original component proof. Private manifest remains local.
set -eu
cd "$(dirname "$0")/../.."
M=${1:?MS20 manifest};OUT=${2:?output};mkdir -p "$OUT";OUT=$(cd "$OUT" && pwd)
python3 tests/host/original_ms20_vectors.py "$M" "$OUT/ms20-vectors.txt"
for arithmetic in native soft;do
 extra='';[ "$arithmetic" != soft ] || extra='-DORIGINAL_USE_SOFT_FMA'
 for mode in normal sanitized;do
  flags='-O2';[ "$mode" != sanitized ] || flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
  ${CC:-cc} -std=c99 $flags $extra -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth tests/host/original_ms20_test.c experiments/original_synth/miami_ms20.c experiments/original_synth/fused_reference.c -lm -o "$OUT/ms20-$arithmetic-$mode"
  "$OUT/ms20-$arithmetic-$mode" "$OUT/ms20-vectors.txt" "$OUT/ms20-$arithmetic-$mode.raw"
 done
done
cmp "$OUT/ms20-native-normal.raw" "$OUT/ms20-soft-normal.raw"
cmp "$OUT/ms20-native-normal.raw" "$OUT/ms20-native-sanitized.raw"
cmp "$OUT/ms20-native-normal.raw" "$OUT/ms20-soft-sanitized.raw"
sha256sum "$OUT"/ms20-*.raw
