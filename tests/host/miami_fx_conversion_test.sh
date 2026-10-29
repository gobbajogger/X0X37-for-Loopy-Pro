#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Inactive stateless conversion; private manifest input, no AU/hardware.
set -eu
cd "$(dirname "$0")/../.."
MANIFEST=${1:?manifest required}; OUT=${2:?output directory required}
mkdir -p "$OUT";OUT=$(cd "$OUT" && pwd)
python3 tests/host/miami_fx_conversion_vectors.py "$MANIFEST" "$OUT/vectors.txt"
for mode in normal sanitized;do
 flags='-O2';[ "$mode" != sanitized ] || flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
 ${CC:-cc} -std=c99 $flags -fno-fast-math -ffp-contract=off -frounding-math -Wall -Wextra -Werror -pedantic -Iexperiments/wobble tests/host/miami_fx_conversion_test.c experiments/wobble/miami_fx_conversion.c experiments/wobble/miami_decimator_component.c -lm -o "$OUT/$mode"
 "$OUT/$mode" "$OUT/vectors.txt"
done
${CC:-cc} -std=c99 -O2 -fno-fast-math -ffp-contract=off -frounding-math -c experiments/wobble/miami_fx_conversion.c -o "$OUT/conversion.o"
size -A "$OUT/conversion.o"
"$OUT/normal" --bench
