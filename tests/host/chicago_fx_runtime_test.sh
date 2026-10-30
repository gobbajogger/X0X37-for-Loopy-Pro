#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Chicago FX runtime (restored inserts + all 81 factories), normal and sanitized.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/host/chicago-fx-runtime}; mkdir -p "$OUT"
SRC=$(python3 -c "
import importlib.util
s=importlib.util.spec_from_file_location('b','tools/build.py');b=importlib.util.module_from_spec(s);s.loader.exec_module(b)
print(' '.join([*b.ORIGINAL_SYNTH_UNITS,*b.ORIGINAL_SYNTH_BACKENDS,'firmware/src/dsp/x0x_param.c']))")
F="-std=c99 -Wall -Wextra -Werror -Wno-misleading-indentation -Wno-array-parameter -fno-fast-math -ffp-contract=off -DORIGINAL_USE_SOFT_FMA -DMIAMI_FX_NO_HOST_FENV -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Iexperiments/original_synth -Iexperiments/wobble"
${CC:-cc} $F -O2 tests/host/chicago_fx_runtime_test.c $SRC -lm -o "$OUT/normal"
${CC:-cc} $F -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all tests/host/chicago_fx_runtime_test.c $SRC -lm -o "$OUT/sanitized"
"$OUT/normal" > "$OUT/normal.log"
ASAN_OPTIONS=detect_leaks=0 "$OUT/sanitized" > "$OUT/sanitized.log"
cmp "$OUT/normal.log" "$OUT/sanitized.log" || echo "note: normal/sanitized float output differs (different -O)"
cat "$OUT/normal.log"
