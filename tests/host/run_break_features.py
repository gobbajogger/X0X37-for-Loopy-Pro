#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile BREAK integration fixtures against the shipping revision-13 engines.

No device access. Private pinned inputs are checked by the production build config.
"""
from pathlib import Path
import importlib.util
import os
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
os.chdir(root)
subprocess.run(['sh', 'host/build_host.sh', '--generate-only'], check=True)
spec = importlib.util.spec_from_file_location('build_config', root / 'tools/build.py')
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)
build.require_original_synth_sources()
out = root / 'build/host'
scanner = (root / 'firmware/src/app/main_fm1.c').read_text()
a = scanner.index('static void smk_wheels(void)\n')
z = scanner.index('\n}\n', a) + 3
(out / 'break_smk_wheels.inc').write_text(scanner[a:z])
subprocess.run([sys.executable, 'tests/host/extract_native_break_service.py'], check=True)
units = ['firmware/src/' + q for q in build.O2_UNITS if q != 'dsp/bass303.c']
units += ['firmware/src/app/factory_banks.c', *build.ORIGINAL_SYNTH_UNITS, *build.ORIGINAL_SYNTH_BACKENDS]
flags = [os.environ.get('CC', 'cc'), '-std=c99', '-O2', '-ffp-contract=off', '-fno-fast-math',
         '-DX0X_HOST', '-DBOARD_SMK37=1', '-DX0X_ORIGINAL_SYNTH=1', '-DX0X_WOBBLE=0', '-DX0X_DEV_WOBBLE=0',
         '-D_POSIX_C_SOURCE=200809L', '-D_DARWIN_C_SOURCE', '-DORIGINAL_USE_SOFT_FMA', '-DMIAMI_FX_NO_HOST_FENV',
         '-Wno-unused-function', '-Ifirmware/src', '-Ifirmware/src/dsp', '-Ibuild/gen', '-Ibuild/host',
         '-Iexperiments/original_synth', '-Iexperiments/wobble', '-ffunction-sections', '-fdata-sections']
if os.environ.get('BREAK_SANITIZE') == '1':
    flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
link = '-Wl,-dead_strip' if sys.platform == 'darwin' else '-Wl,--gc-sections'
for fixture in sys.argv[1:] or ['controls', 'storage']:
    assert fixture in ('controls', 'storage')
    binary = out / ('break_features_' + fixture)
    subprocess.run(flags + ['tests/host/adaptive_break_' + fixture + '_test.c', *units,
                            link, '-lm', '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
