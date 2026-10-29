#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Exact codec delta only. Baseline paths are immutable own-source proof inputs.
set -eu
cd "$(dirname "$0")/../.."
BASE=${1:?baseline source directory};OUT=${2:?output directory};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
RENAMES='-Doriginal_miami_program_encode=baseline_original_miami_program_encode -Doriginal_miami_program_decode=baseline_original_miami_program_decode -Doriginal_chicago_program_encode=baseline_original_chicago_program_encode -Doriginal_chicago_program_decode=baseline_original_chicago_program_decode -Doriginal_miami_voice_snapshot_capture=baseline_original_miami_voice_snapshot_capture -Doriginal_miami_voice_snapshot_encode=baseline_original_miami_voice_snapshot_encode -Doriginal_miami_voice_snapshot_decode=baseline_original_miami_voice_snapshot_decode -Doriginal_chicago_voice_snapshot_capture=baseline_original_chicago_voice_snapshot_capture -Doriginal_chicago_voice_snapshot_encode=baseline_original_chicago_voice_snapshot_encode -Doriginal_chicago_voice_snapshot_decode=baseline_original_chicago_voice_snapshot_decode'
for mode in normal sanitized;do
 flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
 common="$flags -std=c99 -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -Iexperiments/original_synth"
 for unit in program_envelope miami_voice_snapshot chicago_voice_snapshot;do
  ${CC:-cc} $common $RENAMES -c "$BASE/baseline_$unit.c" -o "$OUT/baseline-$unit-$mode.o"
 done
 exe="$OUT/codecs-$mode"
 ${CC:-cc} $common tests/host/original_state_codec_equivalence_test.c "$OUT/baseline-program_envelope-$mode.o" "$OUT/baseline-miami_voice_snapshot-$mode.o" "$OUT/baseline-chicago_voice_snapshot-$mode.o" experiments/original_synth/state_codec.c experiments/original_synth/program_envelope.c experiments/original_synth/miami_voice_snapshot.c experiments/original_synth/chicago_voice_snapshot.c experiments/original_synth/miami_program.c experiments/original_synth/chicago_program.c experiments/original_synth/fused_reference.c -DORIGINAL_USE_SOFT_FMA -lm -o "$exe"
 "$exe" > "$exe.log"
done
cmp "$OUT/codecs-normal.log" "$OUT/codecs-sanitized.log"
cat "$OUT/codecs-normal.log"
