#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Complete-state pre/post implementation comparison; no original/runtime/QA claim.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
SRC='experiments/original_synth/chicago_fx2_bank.c experiments/original_synth/chicago_modulation.c experiments/original_synth/chicago_dynamics.c experiments/original_synth/chicago_smallfx.c experiments/original_synth/chicago_fx_history.c experiments/original_synth/fused_reference.c experiments/wobble/miami_decimator_adapter.c experiments/wobble/miami_decimator_component.c experiments/wobble/miami_fx_conversion.c'
for mode in normal sanitized;do
 flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
 exe="$OUT/events-$mode";rm -f "$exe"
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_chicago_bank_events_test.c tests/host/fixtures/original_chicago_bank_events_baseline.c $SRC -lm -o "$exe"
 "$exe" > "$exe.log"
done
cmp "$OUT/events-normal.log" "$OUT/events-sanitized.log"
cat "$OUT/events-normal.log"
