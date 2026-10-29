#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Actual allocator/voice composition; explicit test-only global resolver.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
SRC='experiments/original_synth/chicago_global_stage.c experiments/original_synth/chicago_global_mg.c experiments/original_synth/chicago_voice_owner.c experiments/original_synth/chicago_allocator.c experiments/original_synth/chicago_voice.c experiments/original_synth/chicago_program.c experiments/original_synth/chicago_mp4.c experiments/original_synth/chicago_packed.c experiments/original_synth/lossless_table.c experiments/original_synth/voice_gain.c experiments/original_synth/oscillator_motion.c experiments/original_synth/miami_ms20.c experiments/original_synth/control_math.c experiments/original_synth/fused_reference.c experiments/wobble/miami_fx_conversion.c'
for arithmetic in native soft;do
 for mode in normal sanitized;do
  flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
  [ "$arithmetic" != soft ]||flags="$flags -DORIGINAL_USE_SOFT_FMA"
  exe="$OUT/owner-$arithmetic-$mode";rm -f "$exe"
  ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_chicago_voice_owner_test.c $SRC -lm -o "$exe"
  "$exe" > "$exe.log"
 done
done
for x in native-sanitized soft-normal soft-sanitized;do cmp "$OUT/owner-native-normal.log" "$OUT/owner-$x.log";done
cat "$OUT/owner-native-normal.log"
