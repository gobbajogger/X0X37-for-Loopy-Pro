#!/bin/sh
# Prints the compiler flags and source units of the original Chicago/Miami engines
# (the only engine profile; bass303 and WBL1 were removed). Host test scripts put
# $(sh tests/host/original_units.sh) where they used to name bass303.c / wobble.c.
set -e
python3 - <<'PY'
import importlib.util
spec = importlib.util.spec_from_file_location('x0x_build', 'tools/build.py')
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)
build.require_original_synth_sources()
print(' '.join(['-DX0X_ORIGINAL_SYNTH=1', '-DX0X_WOBBLE=0', '-DX0X_DEV_WOBBLE=0', '-fno-fast-math',
                '-DORIGINAL_USE_SOFT_FMA', '-DMIAMI_FX_NO_HOST_FENV',
                '-Iexperiments/original_synth', '-Iexperiments/wobble',
                *build.ORIGINAL_SYNTH_UNITS, *build.ORIGINAL_SYNTH_BACKENDS]))
PY
