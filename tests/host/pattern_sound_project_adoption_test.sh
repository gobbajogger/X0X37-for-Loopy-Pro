#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/pattern-sound-project-adoption}
mkdir -p "$OUT"
DARWIN=
[ "$(uname -s)" != Darwin ] || DARWIN=-D_DARWIN_C_SOURCE
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
${CC:-cc} -std=c99 -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -D_POSIX_C_SOURCE=200809L $DARWIN -DX0X_HOST -DX0X_ORIGINAL_SYNTH=1 -DX0X_DEV_WOBBLE=0 -DX0X_WOBBLE=0 \
    -DORIGINAL_USE_SOFT_FMA -DMIAMI_FX_NO_HOST_FENV -fno-fast-math -ffp-contract=off \
    -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -Iexperiments/original_synth -Iexperiments/wobble \
    tests/host/pattern_sound_project_adoption_test.c $U -lm -o "$OUT/test" > "$OUT/compile.log" 2>&1 || { cat "$OUT/compile.log"; exit 1; }
"$OUT/test"
