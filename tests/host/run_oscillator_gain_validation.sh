#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
out=${1:-build/host/oscillator-gain-validation}
mkdir -p "$out"
for mode in normal sanitized; do
    flags='-O2'
    if [ "$mode" = sanitized ]; then
        flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
    fi
    ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -fno-fast-math -ffp-contract=off \
        -Iexperiments/original_synth -Iexperiments/wobble \
        tests/host/oscillator_gain_validation_test.c experiments/wobble/miami_fx_conversion.c \
        -lm -o "$out/$mode"
    "$out/$mode"
done
