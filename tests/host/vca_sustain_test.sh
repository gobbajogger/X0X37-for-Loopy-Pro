#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Old/new bit comparison with native and firmware software FMA, O2 and Os.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/host/vca-sustain}
mkdir -p "$OUT"
CC=${CC:-cc}
for opt in O2 Os; do
 for fma in hard soft; do
  extra=""
  define=""
  if [ "$fma" = soft ]; then
   extra="experiments/original_synth/fused_reference.c"
   define="-DORIGINAL_USE_SOFT_FMA"
  fi
  "$CC" -std=c99 "-$opt" -Wall -Wextra -Werror -fno-fast-math \
   -ffp-contract=off -Iexperiments/original_synth $define \
   tests/host/vca_sustain_test.c experiments/original_synth/control_math.c \
   $extra -lm -o "$OUT/$opt-$fma"
  "$OUT/$opt-$fma"
 done
done
