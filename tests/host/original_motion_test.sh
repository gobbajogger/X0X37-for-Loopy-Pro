#!/bin/sh
# Inactive explicit-state components, no owner device, active engine or original audio.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?fixture/output directory};OUT=$(cd "$OUT"&&pwd)
SRC='experiments/original_synth/oscillator_motion.c experiments/original_synth/chicago_mp4.c experiments/original_synth/miami_ms20.c experiments/original_synth/fused_reference.c experiments/wobble/miami_fx_conversion.c'
for arithmetic in native soft;do
 for mode in normal sanitized;do
  flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
  [ "$arithmetic" != soft ]||flags="$flags -DORIGINAL_USE_SOFT_FMA"
  ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_motion_test.c $SRC -lm -o "$OUT/$arithmetic-$mode"
  "$OUT/$arithmetic-$mode" "$OUT" > "$OUT/$arithmetic-$mode.log"
 done
done
