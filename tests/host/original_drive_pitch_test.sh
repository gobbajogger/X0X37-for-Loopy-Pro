#!/bin/sh
# Local inactive state/arithmetic tests, no original binary/device execution.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?fixture directory};OUT=$(cd "$OUT"&&pwd)
SRC='experiments/original_synth/chicago_mp4.c experiments/original_synth/chicago_packed.c experiments/original_synth/chicago_drive.c experiments/original_synth/miami_pitch.c experiments/original_synth/lossless_table.c experiments/original_synth/oscillator_motion.c experiments/original_synth/fused_reference.c experiments/wobble/miami_fx_conversion.c'
for arithmetic in native soft;do
 for mode in normal sanitized;do
  flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
  [ "$arithmetic" != soft ]||flags="$flags -DORIGINAL_USE_SOFT_FMA"
  ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_drive_pitch_test.c $SRC -lm -o "$OUT/$arithmetic-$mode"
  "$OUT/$arithmetic-$mode" "$OUT" > "$OUT/$arithmetic-$mode.log"
 done
done
