#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Exhaustive comparison with a06e20189's two-lookup tanh body at -O0 and -O2.
# Covers all 4,278,190,082 non-NaN binary32 inputs, including infinities and zeros.
set -eu
cd "$(dirname "$0")/../.."
OUT=build/909-tanh-const-proof
mkdir -p "$OUT"
python3 tools/gen_drum_samples.py
for OPT in 0 2; do
    "${CC:-cc}" -O"$OPT" -std=c99 -DX0X_HOST -ffp-contract=off -fno-fast-math -fno-builtin \
        -Wall -Wextra -Werror -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
        tests/host/drum909_tanh_pair_test.c -lm -o "$OUT/tanh-O$OPT"
    "$OUT/tanh-O$OPT"
done
