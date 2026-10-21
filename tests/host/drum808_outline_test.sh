#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Compare original inlining policy with study outlining, including DSP state.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
RENAMES=$(grep -E '^(void|int|const x0x_param_t \*)' firmware/src/dsp/drum808.h | sed -n 's/.*\b\(drum808_[a-z_]*\)(.*/-D\1=baseline_\1/p' | tr '\n' ' ')
FLAGS='-O2 -std=c99 -ffp-contract=off -Wall -Wextra -Werror -Ifirmware/src/dsp'
# shellcheck disable=SC2086
${CC:-cc} $FLAGS $RENAMES -Ddrum808_quiet=baseline_drum808_quiet -DX0X_SMC_VENDOR_TEST=0 -c firmware/src/dsp/drum808.c -o "$OUT/base.o"
# shellcheck disable=SC2086
${CC:-cc} $FLAGS -DX0X_SMC_VENDOR_TEST=1 -c firmware/src/dsp/drum808.c -o "$OUT/new.o"
# shellcheck disable=SC2086
${CC:-cc} $FLAGS tests/host/drum808_outline_test.c "$OUT/base.o" "$OUT/new.o" -lm -o "$OUT/test"
"$OUT/test"
