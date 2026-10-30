#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Source from repository root for whole-app host tests of the supported engines.
# Keep flags and sources shared with the shipping build composition.
ORIGINAL_FLAGS='-DX0X_ORIGINAL_SYNTH=1 -DX0X_WOBBLE=0 -DX0X_DEV_WOBBLE=0 -DORIGINAL_USE_SOFT_FMA -DMIAMI_FX_NO_HOST_FENV -fno-fast-math -ffp-contract=off -Iexperiments/original_synth -Iexperiments/wobble -Wno-error=missing-braces'
ORIGINAL_SOURCES=$(python3 - <<'PY'
import importlib.util
spec = importlib.util.spec_from_file_location('build', 'tools/build.py')
b = importlib.util.module_from_spec(spec)
spec.loader.exec_module(b)
b.synth_build_config()
b.require_original_synth_sources()
print(' '.join(['firmware/src/' + p for p in b.O2_UNITS] +
               ['firmware/src/app/factory_banks.c', *b.ORIGINAL_SYNTH_UNITS, *b.ORIGINAL_SYNTH_BACKENDS]))
PY
) || return $?
