#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Assigned selected voice, immutable table ownership, reference scan comparison.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
SRC='experiments/original_synth/miami_selected_voice.c experiments/original_synth/miami_program.c experiments/original_synth/miami_pitch.c experiments/original_synth/voice_gain.c experiments/original_synth/oscillator_motion.c experiments/original_synth/miami_ms20.c experiments/original_synth/control_math.c experiments/original_synth/fused_reference.c experiments/wobble/miami_fx_conversion.c'
for ownership in copy borrowed;do
 for arithmetic in native soft;do
  for mode in normal sanitized;do
   flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
   [ "$arithmetic" != soft ]||flags="$flags -DORIGINAL_USE_SOFT_FMA"
   [ "$ownership" != borrowed ]||flags="$flags -DMIAMI_VOICE_BORROW_TABLES"
   exe="$OUT/$ownership-$arithmetic-$mode"
   ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_miami_selected_voice_test.c $SRC -lm -o "$exe"
   "$exe" "$exe.raw" > "$exe.log"
  done
 done
done
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -DMIAMI_VOICE_CHECKED_TABLE_PREP -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_miami_selected_voice_test.c $SRC -lm -o "$OUT/checked"
"$OUT/checked" "$OUT/checked.raw" > "$OUT/checked.log"
for run in copy-native-sanitized copy-soft-normal copy-soft-sanitized borrowed-native-normal borrowed-native-sanitized borrowed-soft-normal borrowed-soft-sanitized checked;do cmp "$OUT/copy-native-normal.raw" "$OUT/$run.raw";done
printf 'PASS all9 selected-voice builds; complete sample/state bytes equal, native/software FMA, ownership modes and full-scan reference\n'
