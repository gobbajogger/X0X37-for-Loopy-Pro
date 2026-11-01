#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Compile the same voice as the production cache and a separate uncached
# reference; real oscillator, filter, FMA and gain paths are shared.
set -eu
cd "$(dirname "$0")/../.."
CC=${CC:-cc}
OUT=${X0X_CHICAGO_COEF_CACHE_OUT:-build/host/chicago-coef-cache}
mkdir -p "$OUT"
CFLAGS="-O2 -std=c99 -fno-fast-math -ffp-contract=off -ffunction-sections -fdata-sections -DORIGINAL_USE_SOFT_FMA -Iexperiments/original_synth -Iexperiments/wobble -Ifirmware/src -Ifirmware/src/dsp"
case "$(uname -s)" in
 Darwin) GC="-Wl,-dead_strip" ;;
 *) GC="-Wl,--gc-sections" ;;
esac
set --
for name in $(grep -oE 'original_chicago_voice_[a-z0-9_]+[(]' experiments/original_synth/chicago_voice.c | sed 's/($//' | sort -u); do
 case "$name" in original_chicago_voice_kernel|original_chicago_voice_span) continue ;; esac
 set -- "$@" "-D${name}=uncached_${name}"
done
$CC $CFLAGS -DCHICAGO_COEF_CACHE_REFERENCE "$@" -c experiments/original_synth/chicago_voice.c -o "$OUT/uncached_voice.o"
$CC $CFLAGS \
 tests/host/chicago_coef_cache_test.c \
 experiments/original_synth/chicago_voice.c \
 experiments/original_synth/chicago_program.c \
 experiments/original_synth/chicago_mp4.c \
 experiments/original_synth/chicago_voice_snapshot.c \
 experiments/original_synth/state_codec.c \
 experiments/original_synth/control_math.c \
 experiments/original_synth/miami_ms20.c \
 experiments/original_synth/oscillator_motion.c \
 experiments/original_synth/voice_gain.c \
 experiments/original_synth/fused_reference.c \
 experiments/wobble/miami_fx_conversion.c \
 "$OUT/uncached_voice.o" $GC -lm -o "$OUT/test"
"$OUT/test"
