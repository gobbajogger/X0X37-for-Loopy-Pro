#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
DARWIN=
[ "$(uname -s)" != Darwin ] || DARWIN=-D_DARWIN_C_SOURCE
OUT=${1:?output directory}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
sh host/build_host.sh --generate-only > "$OUT/assets.log" 2>&1
U=$(python3 - <<'PY'
import importlib.util
spec = importlib.util.spec_from_file_location('build', 'tools/build.py')
b = importlib.util.module_from_spec(spec)
spec.loader.exec_module(b)
u = ['firmware/src/' + p for p in b.O2_UNITS if p != 'dsp/bass303.c']
u += ['firmware/src/app/factory_banks.c', *b.ORIGINAL_SYNTH_UNITS, *b.ORIGINAL_SYNTH_BACKENDS]
print(' '.join(u))
PY
)
printf '%s\n' "$U" > "$OUT/sources.txt"
for mode in normal sanitized; do
    mkdir -p "$OUT/$mode-images"
    FLAGS='-O2'
    [ "$mode" != sanitized ] || FLAGS='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
    # Existing whole-app GCC weak-inline/attribute warnings remain visible.
    # Focused new modules are separately checked with -Werror.
    ${CC:-cc} -std=c99 $FLAGS -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
        -D_POSIX_C_SOURCE=200809L $DARWIN ${X0X_HOST_DEFS:-} -DX0X_HOST -DX0X_ORIGINAL_SYNTH=1 -DX0X_DEV_WOBBLE=0 -DX0X_WOBBLE=0 \
        -DORIGINAL_USE_SOFT_FMA -DMIAMI_FX_NO_HOST_FENV -fno-fast-math -ffp-contract=off \
        -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -Iexperiments/original_synth -Iexperiments/wobble \
        tests/host/original_release_integration_test.c $U -lm -o "$OUT/$mode" > "$OUT/$mode.compile.log" 2>&1
    "$OUT/$mode" "$OUT/$mode-images" > "$OUT/$mode.log" 2>&1
done
cmp "$OUT/normal.log" "$OUT/sanitized.log"
for choice in 0 1 2 3 4 5 6; do
    cmp "$OUT/normal-images/chicago-fx-$choice.png" "$OUT/sanitized-images/chicago-fx-$choice.png"
done
cat "$OUT/normal.log"
