#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Check the same original arithmetic with hardware and software FMA.
set -eu
cd "$(dirname "$0")/../.."
OUT=build/host/miami-coef-cache
if [ "$#" -gt 0 ]; then OUT=$1; fi
mkdir -p "$OUT"
CC=${CC:-cc}
for opt in O2 Os; do
 for fma in hard soft; do
  opts=""
  extra=""
  if [ "$fma" = soft ]; then
   opts="-DORIGINAL_USE_SOFT_FMA"
   extra="experiments/original_synth/fused_reference.c"
  fi
  "$CC" -std=c99 "-$opt" -Wall -Wextra -Werror \
    -Wno-unused-function -Wno-missing-braces \
    -fno-fast-math -ffp-contract=off -frounding-math \
    -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -Iexperiments/original_synth -Iexperiments/wobble $opts \
    tests/host/miami_coef_cache_test.c \
    experiments/original_synth/oscillator_motion.c \
    experiments/original_synth/miami_pitch.c \
    experiments/original_synth/miami_ms20.c \
    experiments/original_synth/voice_gain.c \
    experiments/wobble/miami_fx_conversion.c $extra -lm \
    -o "$OUT/$opt-$fma"
  "$OUT/$opt-$fma"
 done
done
