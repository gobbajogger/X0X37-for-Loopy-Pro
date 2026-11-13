#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Exhaustive paired table lookup proof, independent of historical Git refs.
set -eu
cd "$(dirname "$0")/../.."
OUT=build/host/drum909-tanh-pair
mkdir -p "$OUT" build/gen
if [ ! -f build/gen/x0x_drum_tables.h ]; then
    python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h
fi
"${CC:-cc}" -O2 -std=c99 -DX0X_HOST -ffp-contract=off -fno-fast-math -fno-builtin \
    -Wall -Wextra -Werror -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    tests/host/drum909_tanh_pair_test.c -lm -o "$OUT/tanh-pair"
"$OUT/tanh-pair"
