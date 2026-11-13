#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Compare the integer-rate sampler shortcut with the pinned original engine.
# Requires cc, nm, objcopy, python3 and the local base Git object. No network.
set -eu
cd "$(dirname "$0")/../.."
BASE=${BASE:-639439adaa5e39ada564a1f9f7b4150a3909df47}
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
: "${CC:=cc}"
git show "$BASE:firmware/src/dsp/drum909.c" > "$OUT/base.c"
for opt in ${OPT_LEVELS:-O0 O2}; do
    flags="-std=c99 -$opt -ffp-contract=off -fno-fast-math -DX0X_HOST -Wall -Wextra -Werror -Ifirmware/src/dsp -Ibuild/gen"
    $CC $flags -c "$OUT/base.c" -o "$OUT/base.o"
    nm -g --defined-only "$OUT/base.o" | awk '{print $3 " ref_" $3}' > "$OUT/symbols"
    objcopy --redefine-syms="$OUT/symbols" "$OUT/base.o" "$OUT/ref.o"
    $CC $flags -c firmware/src/dsp/drum909.c -o "$OUT/candidate.o"
    $CC $flags tests/host/drum909_integral_test.c "$OUT/ref.o" "$OUT/candidate.o" firmware/src/dsp/x0x_param.c -lm -o "$OUT/test"
    echo "$opt"
    "$OUT/test"
    size "$OUT/base.o" "$OUT/candidate.o"
done
