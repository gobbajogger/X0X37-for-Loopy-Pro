#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Actual released UI with original-control semantics: normal/sanitized + PNG equality.
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
s=importlib.util.spec_from_file_location('build','tools/build.py')
b=importlib.util.module_from_spec(s);s.loader.exec_module(b)
u=['firmware/src/'+p for p in b.O2_UNITS if p!='dsp/bass303.c']
u+=['firmware/src/app/factory_banks.c',*b.ORIGINAL_SYNTH_UNITS,*b.ORIGINAL_SYNTH_BACKENDS]
print(' '.join(u))
PY
)
printf '%s\n' "$U" > "$OUT/sources.txt"
for mode in normal sanitized; do
    mkdir -p "$OUT/$mode-images"
    FLAGS='-O2 -fstack-usage'
    [ "$mode" != sanitized ] || FLAGS='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
    ${CC:-cc} -std=c99 $FLAGS -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
        -D_POSIX_C_SOURCE=200809L $DARWIN ${X0X_HOST_DEFS:-} -DX0X_HOST -DX0X_ORIGINAL_SYNTH=1 -DX0X_DEV_WOBBLE=0 -DX0X_WOBBLE=0 \
        -DORIGINAL_USE_SOFT_FMA -DMIAMI_FX_NO_HOST_FENV -fno-fast-math -ffp-contract=off \
        -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -Iexperiments/original_synth -Iexperiments/wobble \
        ${ORIGINAL_UI_TEST_FILE:-tests/host/original_release_ui_test.c} $U -lm -o "$OUT/$mode" > "$OUT/$mode.compile.log" 2>&1
    ASAN_OPTIONS=detect_leaks=0 "$OUT/$mode" "$OUT/$mode-images" > "$OUT/$mode.log" 2>&1
done
cmp "$OUT/normal.log" "$OUT/sanitized.log"
for f in "$OUT/normal-images/"*.png; do cmp "$f" "$OUT/sanitized-images/$(basename "$f")"; done
cat "$OUT/normal.log"
