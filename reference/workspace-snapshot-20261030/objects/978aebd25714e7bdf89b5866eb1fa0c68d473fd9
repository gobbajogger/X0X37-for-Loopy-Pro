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
    dsp_dir = ROOT / 'firmware' / 'src' / 'dsp'
    cc = os.environ.get('CC', 'cc')
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        original_argv = sys.argv[:]
        results = {}
        for kind in ('baseline', 'packed'):
            out = tmp / kind
            gen = out / 'gen'
            shadow = out / 'shadow' / 'dsp'
            gen.mkdir(parents=True)
            shadow.mkdir(parents=True)
            for hdr in dsp_dir.glob('*.h'):
                if kind == 'baseline':
                    try:
                        (shadow / hdr.name).write_bytes(old('firmware/src/dsp/' + hdr.name))
                        continue
                    except subprocess.CalledProcessError:
                        pass
                (shadow / hdr.name).write_bytes(hdr.read_bytes())
            if kind == 'packed':
                (shadow / 'drum909_dsp.h').write_bytes((dsp_dir / 'drum909_dsp.h').read_bytes())

            script = ROOT / 'tools/gen_drum_samples.py'
            code = old('tools/gen_drum_samples.py') if kind == 'baseline' else script.read_bytes()
            sys.path.insert(0, str(ROOT / 'tools'))
            ns = {'__file__': str(script), '__name__': 'generator_test'}
            exec(compile(code, str(script), 'exec'), ns)
            sys.argv = [str(script), str(gen / 'x0x_drum_samples.h')]
            ns['main']()
            (shadow / 'x0x_drum_tables.h').write_bytes((gen / 'x0x_drum_tables.h').read_bytes())
            (shadow / 'x0x_drum_samples.h').write_bytes((gen / 'x0x_drum_samples.h').read_bytes())

            objects = []
            for name in ('drum909', 'fxbus'):
                path = 'firmware/src/dsp/' + name + '.c'
                src = out / (name + '.c')
                src.write_bytes(old(path) if kind == 'baseline' else (ROOT / path).read_bytes())
                obj = out / (name + '.o')
                run([cc, '-O2', '-ffp-contract=off', '-std=c99',
                     '-Wall', '-Wextra', '-Werror', '-Wdouble-promotion',
                     '-I' + str(shadow), '-I' + str(gen),
                     '-c', str(src), '-o', str(obj)])
                objects.append(str(obj))
            exe = out / 'audio'
            run([cc, '-O2', '-ffp-contract=off',
                 '-I' + str(out / 'shadow'), '-I' + str(gen),
                 str(ROOT / 'tests/host/pot_packing_audio_test.c'), *objects, '-lm',
                 '-o', str(exe)])
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
