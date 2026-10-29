#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Original profile: real storage/project/engine migration. No device I/O.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/host/project-original-migration}
mkdir -p "$OUT"
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
DARWIN=
[ "$(uname -s)" != Darwin ] || DARWIN=-D_DARWIN_C_SOURCE
# shellcheck disable=SC2086
${CC:-cc} -std=c99 -O1 -g -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    $DARWIN ${SAN_FLAGS:-} -DX0X_HOST -DX0X_ORIGINAL_SYNTH=1 -DX0X_DEV_WOBBLE=0 -DX0X_WOBBLE=0 \
    -DORIGINAL_USE_SOFT_FMA -DMIAMI_FX_NO_HOST_FENV -fno-fast-math -ffp-contract=off \
    -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -Iexperiments/original_synth -Iexperiments/wobble \
    tests/host/project_original_migration_test.c $U -lm -o "$OUT/test" > "$OUT/compile.log" 2>&1
"$OUT/test"
