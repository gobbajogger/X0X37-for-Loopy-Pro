#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile exhaustive bit-exact raw/compact oracle and malformed-input checks."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='chicago-compact-') as tmp:
    tmp = Path(tmp)
    subprocess.run([sys.executable, str(ROOT/'tools/gen_chicago_compact.py'),
                    str(ROOT/'experiments/original_synth/chicago_packed_private.h'),
                    str(tmp/'chicago_compact_private.h')], check=True)
    cc = os.environ.get('CC', 'cc')
    for flags in (['-O2'], ['-O1', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']):
        output = tmp/'test'
        subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror', *flags,
                        '-I'+str(tmp), '-I'+str(ROOT/'experiments/original_synth'),
                        str(ROOT/'tests/chicago_compact_headers.c'),
                        str(ROOT/'tests/chicago_compact_raw.c'),
                        str(ROOT/'experiments/original_synth/lossless_table.c'),
                        '-o', str(output)], check=True)
        subprocess.run([str(output)], check=True)
        print('PASS: 32770 exact values + public cached lookups + invalid inputs:', ' '.join(flags))
