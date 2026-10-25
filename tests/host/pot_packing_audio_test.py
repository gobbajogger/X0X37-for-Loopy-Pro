#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compare exhaustive 909/FX setter audio to exact prepacking source on this host."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BASE = 'e75fb2a584aa22978f352697a5e36fc9f886c3c5'
def run(args, **kw):
    return subprocess.run(args, check=True, cwd=ROOT, **kw)
def old(path):
    return subprocess.check_output(['git', 'show', BASE + ':' + path], cwd=ROOT)

def main():
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        original_argv = sys.argv[:]
        results = {}
        for kind in ('baseline', 'packed'):
            out = tmp / kind
            out.mkdir()
            script = ROOT / 'tools/gen_drum_samples.py'
            code = old('tools/gen_drum_samples.py') if kind == 'baseline' else script.read_bytes()
            sys.path.insert(0, str(ROOT / 'tools'))
            ns = {'__file__': str(script), '__name__': 'generator_test'}
            exec(compile(code, str(script), 'exec'), ns)
            sys.argv = [str(script), str(out / 'x0x_drum_samples.h')]
            ns['main']()
            objects = []
            for name in ('drum909', 'fxbus'):
                path = 'firmware/src/dsp/' + name + '.c'
                src = out / (name + '.c')
                src.write_bytes(old(path) if kind == 'baseline' else (ROOT / path).read_bytes())
                obj = out / (name + '.o')
                run([os.environ.get('CC', 'cc'), '-O2', '-ffp-contract=off', '-std=c99',
                     '-Wall', '-Wextra', '-Werror', '-Wdouble-promotion',
                     '-I' + str(ROOT / 'firmware/src/dsp'), '-I' + str(out),
                     '-c', str(src), '-o', str(obj)])
                objects.append(str(obj))
            exe = out / 'audio'
            run([os.environ.get('CC', 'cc'), '-O2', '-ffp-contract=off',
                 '-I' + str(ROOT / 'firmware/src'), '-I' + str(out),
                 str(ROOT / 'tests/host/pot_packing_audio_test.c'), *objects, '-lm', '-o', str(exe)])
            with (out / 'audio.bin').open('wb') as f:
                run([str(exe)], stdout=f)
            data = (out / 'audio.bin').read_bytes()
            results[kind] = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
        sys.argv = original_argv
        assert results['baseline'] == results['packed'], results
        print(json.dumps({'status': 'PASS_BIT_IDENTICAL_STEREO_AUDIO', 'base': BASE,
                          'scope': 'every 909 and FX parameter, values -1..128, 16 stereo samples per setter, all drum voices triggered',
                          'results': results}, indent=2))
if __name__ == '__main__':
    main()
