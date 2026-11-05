#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile actual pinned and working DSP units separately; print every cc command."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BASE = 'e9e6643e612b9fb4e968a77d016f3bb24213dc88'

def run(args):
    print(shlex.join(map(str, args)), flush=True)
    subprocess.run(args, cwd=ROOT, check=True)

def main():
    unit = sys.argv[1]
    assert unit in ('master', 'drum808', 'fxbus')
    source = 'firmware/src/dsp/' + unit + '.c'
    old = subprocess.check_output(['git', 'show', BASE + ':' + source], cwd=ROOT, text=True)
    symbols = sorted(set(re.findall(r'\b(' + unit + r'_\w+)\s*\(', old)))
    with tempfile.TemporaryDirectory(prefix='a3-' + unit + '-') as directory:
        tmp = Path(directory)
        flags = [os.environ.get('CC', 'cc'), '-std=c99', '-ffp-contract=off', '-fno-fast-math',
                 '-Wall', '-Wextra', '-Werror', '-DX0X_HOST', '-Ifirmware/src', '-Ifirmware/src/dsp', '-I' + str(tmp)]
        if unit == 'fxbus':
            run(['python3', 'tools/gen_drum_samples.py', str(tmp / 'x0x_drum_samples.h')])
        helpers = {'master': 'limit_step(master_t *m, float a) { limit_env(m, a); }',
                   'drum808': 'tail_step(drum808_t *d, int l, const float *x, int n) { lane_tail(d, l, x, n); }'}
        for opt in ('-O0', '-O2'):
            ref = tmp / 'reference.c'
            ref.write_text(old + ('\nvoid reference_' + helpers[unit] + '\n' if unit in helpers else ''))
            candidate = tmp / 'candidate.c'
            candidate.write_text('#include "' + str(ROOT / source) + '"\n' +
                                 ('void candidate_' + helpers[unit] + '\n' if unit in helpers else ''))
            run(flags + [opt] + ['-D' + s + '=reference_' + s for s in symbols] + ['-c', ref, '-o', tmp / 'ref.o'])
            run(flags + [opt, '-c', candidate, '-o', tmp / 'new.o'])
            run(flags + [opt, 'tests/host/a3_' + unit + '_equivalence_test.c', tmp / 'ref.o', tmp / 'new.o',
                         'firmware/src/dsp/x0x_param.c', '-lm', '-o', tmp / 'test'])
            run([tmp / 'test'])
            print(unit + ' ' + opt + ': PASS', flush=True)

if __name__ == '__main__':
    main()
