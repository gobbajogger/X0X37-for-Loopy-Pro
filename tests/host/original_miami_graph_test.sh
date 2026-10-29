#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
SRC='experiments/original_synth/miami_selected_voice.c experiments/original_synth/miami_output.c experiments/original_synth/miami_program.c experiments/original_synth/miami_pitch.c experiments/original_synth/voice_gain.c experiments/original_synth/oscillator_motion.c experiments/original_synth/miami_ms20.c experiments/original_synth/control_math.c experiments/original_synth/master.c experiments/original_synth/miami_fx_chain.c experiments/original_synth/fixed_fx.c experiments/original_synth/fused_reference.c experiments/wobble/miami_decimator_component.c experiments/wobble/miami_decimator_adapter.c experiments/wobble/miami_fx_conversion.c'
for arithmetic in native soft;do
 for mode in normal sanitized;do
  flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
  [ "$arithmetic" != soft ]||flags="$flags -DORIGINAL_USE_SOFT_FMA"
  exe="$OUT/$arithmetic-$mode"
  ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_miami_graph_test.c $SRC -lm -o "$exe"
  "$exe" "$exe.raw" > "$exe.log"
 done
done
for run in native-sanitized soft-normal soft-sanitized;do cmp "$OUT/native-normal.raw" "$OUT/$run.raw";done
printf 'PASS joined selected two-voice Miami stereo/output graph; all4 sample streams exact; no allocator/firmware activation claim\n'
