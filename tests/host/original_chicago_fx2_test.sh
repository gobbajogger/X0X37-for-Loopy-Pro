#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Producer composition check. Dedicated host histories are explicitly outside
# current device admission; public reset projection is not all30 arena parity.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory};mkdir -p "$OUT";OUT=$(cd "$OUT"&&pwd)
SRC='experiments/original_synth/chicago_control_owner.c experiments/original_synth/chicago_scheduler.c experiments/original_synth/chicago_selector.c experiments/original_synth/chicago_arp_control.c experiments/original_synth/chicago_allocator.c experiments/original_synth/chicago_voice.c experiments/original_synth/chicago_mp4.c experiments/original_synth/chicago_packed.c experiments/original_synth/lossless_table.c experiments/original_synth/voice_gain.c experiments/original_synth/oscillator_motion.c experiments/original_synth/miami_ms20.c experiments/original_synth/control_math.c experiments/original_synth/chicago_fx2_wrapper.c experiments/original_synth/chicago_fx2_bank.c experiments/original_synth/chicago_engine_output.c experiments/original_synth/chicago_program.c experiments/original_synth/chicago_program_output.c experiments/original_synth/chicago_output.c experiments/original_synth/chicago_drive.c experiments/original_synth/master.c experiments/original_synth/chicago_dynamics.c experiments/original_synth/chicago_smallfx.c experiments/original_synth/chicago_modulation.c experiments/original_synth/chicago_fx_history.c experiments/original_synth/fused_reference.c experiments/wobble/miami_fx_conversion.c experiments/wobble/miami_decimator_adapter.c experiments/wobble/miami_decimator_component.c'
for arithmetic in native soft;do
 for mode in normal sanitized;do
  flags='-O2';[ "$mode" != sanitized ]||flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
  [ "$arithmetic" != soft ]||flags="$flags -DORIGINAL_USE_SOFT_FMA"
  exe="$OUT/fx2-$arithmetic-$mode";rm -f "$exe"
  ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off -frounding-math -Iexperiments/original_synth -Iexperiments/wobble tests/host/original_chicago_fx2_test.c $SRC -lm -o "$exe"
  "$exe" > "$exe.log"
 done
done
for x in native-sanitized soft-normal soft-sanitized;do cmp "$OUT/fx2-native-normal.log" "$OUT/fx2-$x.log";done
cat "$OUT/fx2-native-normal.log"
