#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Focused actual Miami output versus the unchanged independent FX chain.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?new output directory required}
if [ -e "$OUT" ]; then
    printf '%s\n' 'Output already exists; retain prior evidence and choose a new directory.' >&2
    exit 2
fi
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
python3 tools/check_original_inputs.py --json > "$OUT/inputs.json"
for arithmetic in native soft; do
    extra=''
    [ "$arithmetic" != soft ] || extra='-DORIGINAL_USE_SOFT_FMA'
    for mode in normal sanitized; do
        flags='-O2'
        [ "$mode" != sanitized ] || flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
        exe="$OUT/$arithmetic-$mode"
        ${CC:-cc} -std=c99 $flags $extra -Wall -Wextra -Werror -pedantic \
            -fno-builtin -fno-fast-math -ffp-contract=off -frounding-math \
            -Iexperiments/original_synth -Iexperiments/wobble \
            tests/host/original_miami_output_test.c \
            experiments/original_synth/miami_program.c \
            experiments/original_synth/fused_reference.c \
            experiments/original_synth/miami_output.c \
            experiments/original_synth/master.c \
            experiments/original_synth/miami_fx_chain.c \
            experiments/original_synth/fixed_fx.c \
            experiments/wobble/miami_decimator_adapter.c \
            experiments/wobble/miami_decimator_component.c \
            experiments/wobble/miami_fx_conversion.c \
            -lm -o "$exe" > "$exe.compile.log" 2>&1
        ASAN_OPTIONS=detect_leaks=0 "$exe" > "$exe.log" 2>&1
        cmp "$OUT/native-normal.log" "$exe.log"
    done
done
cat "$OUT/native-normal.log"
printf '%s\n' 'PASS Miami output: native/software FMA and normal/ASan/UBSan logs identical'
