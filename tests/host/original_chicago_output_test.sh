#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Fixed original Drive/output parity and public-program routing seam. No
# original component activation, target build or missing FX2 replacement.
set -eu
cd "$(dirname "$0")/../.."
REF=${1:?Chicago Drive functional JSON};OUT=${2:?output directory}
mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
python3 tests/host/original_chicago_output_vectors.py "$REF" "$OUT/oracle.txt" > "$OUT/generation.json"
SRC='experiments/original_synth/chicago_output.c experiments/original_synth/chicago_drive.c experiments/original_synth/master.c experiments/original_synth/fused_reference.c experiments/wobble/miami_fx_conversion.c'
for arithmetic in native soft;do
 for mode in normal sanitized;do
  flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
  [ "$arithmetic" != soft ]||flags="$flags -DORIGINAL_USE_SOFT_FMA"
  exe="$OUT/output-$arithmetic-$mode"
  ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_chicago_output_test.c $SRC -lm -o "$exe"
  "$exe" "$OUT/oracle.txt" "$exe.bin" > "$exe.log"
  ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_chicago_routing_test.c experiments/original_synth/chicago_program.c experiments/original_synth/chicago_program_output.c $SRC -lm -o "$OUT/routing-$arithmetic-$mode"
  "$OUT/routing-$arithmetic-$mode" > "$OUT/routing-$arithmetic-$mode.log"
 done
done
for other in native-sanitized soft-normal soft-sanitized;do cmp "$OUT/output-native-normal.bin" "$OUT/output-$other.bin";done
printf 'PASS Chicago original Drive/output rational oracle/fullstate; publicprogramrouting; native/software FMA normal/ASan/UBSan\n'
