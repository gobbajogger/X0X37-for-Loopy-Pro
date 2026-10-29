#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Portable codecs and original reset-order scheduling only; no live storage.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
for mode in normal sanitized;do
 flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
 exe="$OUT/state-$mode"
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -Iexperiments/original_synth tests/host/original_engine_ingress_state_test.c experiments/original_synth/program_envelope.c experiments/original_synth/state_codec.c experiments/original_synth/chicago_voice_snapshot.c experiments/original_synth/miami_voice_snapshot.c experiments/original_synth/miami_startup.c experiments/original_synth/miami_program.c experiments/original_synth/chicago_program.c experiments/original_synth/fused_reference.c -DORIGINAL_USE_SOFT_FMA -lm -o "$exe"
 "$exe" > "$exe.log"
done
cmp "$OUT/state-normal.log" "$OUT/state-sanitized.log"
cat "$OUT/state-normal.log"
