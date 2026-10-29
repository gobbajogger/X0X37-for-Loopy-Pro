#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Actual scanner/CQ/original-engine regression; no native or device actions."""
from pathlib import Path
import argparse, datetime, hashlib, importlib.util, json, os, shutil, subprocess, sys
sys.dont_write_bytecode = True
p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parent
src, out = a.source.resolve(), a.out.resolve()
out.mkdir(parents=True, exist_ok=False)
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
for original, frozen in (('original_wheels_integration_test.c', 'physical-engine-test.c'),
                         ('original_wheels_engine_probe.c', 'engine-observer.c')):
    shutil.copy2(root/original, out/frozen)
text = (src/'firmware/src/app/main_fm1.c').read_text()
start = text.index('static void smk_wheels(void)\n')
end = text.index('\n}\n', start) + 3
(out/'smk_wheels.inc').write_text(text[start:end])
os.chdir(src)
spec = importlib.util.spec_from_file_location('wheel_build', src/'tools/build.py')
b = importlib.util.module_from_spec(spec)
spec.loader.exec_module(b)
production = ['firmware/src/'+q for q in b.O2_UNITS if q != 'dsp/bass303.c']
production += ['firmware/src/app/factory_banks.c', *b.ORIGINAL_SYNTH_UNITS, *b.ORIGINAL_SYNTH_BACKENDS]
assert len(production) == len(set(production))
units = [str(out/'engine-observer.c') if q == 'firmware/src/app/engine.c' else q for q in production]
base = [os.environ.get('CC', 'cc'), '-std=c99', '-Wall', '-Wextra', '-Wno-unused-function', '-Wno-unused-parameter',
        '-D_POSIX_C_SOURCE=200809L', '-DX0X_HOST', '-DX0X_ORIGINAL_SYNTH=1',
        '-DX0X_DEV_WOBBLE=0', '-DX0X_WOBBLE=0', '-DORIGINAL_USE_SOFT_FMA', '-DMIAMI_FX_NO_HOST_FENV',
        '-fno-fast-math', '-ffp-contract=off', '-fno-builtin', '-Ifirmware/src', '-Ifirmware/src/dsp',
        '-Ibuild/gen', '-Iexperiments/original_synth', '-Iexperiments/wobble', '-I'+str(out),
        '-DWHEEL_HOST_SOURCE="'+str(src/'host/x0x_host.c')+'"',
        '-DWHEEL_ENGINE_SOURCE="'+str(src/'firmware/src/app/engine.c')+'"']
inputs = production + ['host/x0x_host.c', 'firmware/src/app/main_fm1.c', 'firmware/src/app/ui.c',
                       'firmware/src/app/project.c', 'firmware/src/app/original_presets.inc']
inputs += ['firmware/src/app/ui_original_controls.h', 'firmware/src/app/original_sound.h',
           'firmware/src/app/original_runtime.h']
before = {q: sha(src/q) for q in inputs}
record = {'source': str(src), 'scope': 'Exact scanner -> actual CQ -> actual original engines; host platform, not device',
          'policy_expected': 'PB and Damper runtime-only', 'production_units': production, 'source_inputs_sha256': before,
          'probe_sha256': sha(out/'physical-engine-test.c'), 'observer_sha256': sha(out/'engine-observer.c'),
          'scanner_sha256': sha(out/'smk_wheels.inc'), 'modes': []}
def save():
    (out/'RECEIPT.json').write_text(json.dumps(record, indent=2)+'\n')
for mode, flags in [('normal', ['-O2']), ('sanitized', ['-O1', '-g', '-fno-omit-frame-pointer',
                           '-fsanitize=address,undefined', '-fno-sanitize-recover=all'])]:
    cmd = base + flags + [str(out/'physical-engine-test.c'), *units, '-lm', '-o', str(out/mode)]
    row = {'mode': mode, 'command': cmd, 'start_utc': datetime.datetime.now(datetime.timezone.utc).isoformat()}
    record['modes'].append(row)
    save()
    with (out/(mode+'-compile.log')).open('wb') as stream:
        result = subprocess.run(cmd, stdout=stream, stderr=subprocess.STDOUT)
    row['compile_exit'] = result.returncode
    save()
    if result.returncode:
        print((out/(mode+'-compile.log')).read_text()[-4000:], flush=True)
        raise SystemExit(result.returncode)
    row['runs'] = []
    env = os.environ.copy()
    env.update(ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1')
    for part in (0, 1):
        for kind in (0, 1):
            log = out/f'{mode}-part{part}-kind{kind}.log'
            with log.open('wb') as stream:
                result = subprocess.run([str(out/mode), str(part), str(kind)], env=env,
                                        stdout=stream, stderr=subprocess.STDOUT)
            row['runs'].append({'part': part, 'kind': kind, 'exit': result.returncode,
                                'log': log.name, 'sha256': sha(log)})
            save()
            print(mode, part, kind, 'exit', result.returncode, log.read_text()[-1200:], flush=True)
            if result.returncode:
                raise SystemExit(result.returncode)
    row['end_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    save()
for part in (0, 1):
    for kind in (0, 1):
        assert (out/f'normal-part{part}-kind{kind}.log').read_bytes() == (out/f'sanitized-part{part}-kind{kind}.log').read_bytes()
assert before == {q: sha(src/q) for q in inputs}
record['state'] = 'POLICY_PASS'
record['source_inputs_unchanged_after'] = True
save()
print('FINAL', record['state'], sha(out/'RECEIPT.json'), flush=True)
