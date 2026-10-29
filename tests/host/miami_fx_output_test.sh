#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Exhaustive int16 output against the independent Fraction oracle. No private
# witness manifest is needed; this does not claim the full input-chain proof.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory required}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
python3 - "$OUT/vectors.txt" <<'PY'
import sys
sys.path.insert(0, 'tests/host')
from miami_fx_conversion_vectors import back
with open(sys.argv[1], 'w') as out:
    for n in range(-32768, 32768):
        out.write(f'O {n} {back(n):08x}\n')
PY
for mode in normal sanitized; do
    flags='-Os'
    [ "$mode" != sanitized ] || flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
    ${CC:-cc} -std=c99 $flags -fno-builtin -DORIGINAL_USE_SOFT_FMA \
        -fno-fast-math -ffp-contract=off -frounding-math \
        -Wall -Wextra -Werror -pedantic -Iexperiments/wobble \
        tests/host/miami_fx_conversion_test.c \
        experiments/wobble/miami_fx_conversion.c \
        experiments/wobble/miami_decimator_component.c \
        experiments/original_synth/fused_reference.c -lm -o "$OUT/$mode"
    "$OUT/$mode" "$OUT/vectors.txt"
done
