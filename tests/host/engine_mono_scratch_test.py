#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compare current supported engine PCM/meters with the exact combined base."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build/mono-scratch'
OUT.mkdir(parents=True, exist_ok=True)
BASE = 'c295af1554f9e80a29864cd013995eccad66cd2c'
spec = importlib.util.spec_from_file_location('build_config', ROOT / 'tools/build.py')
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)
build.synth_build_config()
build.require_original_synth_sources()
units = [str(ROOT / 'firmware/src' / p) for p in build.O2_UNITS if p != 'app/engine.c']
units += ['firmware/src/app/factory_banks.c', *build.ORIGINAL_SYNTH_UNITS, *build.ORIGINAL_SYNTH_BACKENDS]
baseline = OUT / 'baseline.c'
baseline_text = subprocess.check_output(['git', 'show', BASE + ':firmware/src/app/engine.c'], cwd=ROOT, text=True)
candidate_text = (ROOT/'firmware/src/app/engine.c').read_text()
call = '        render_sub(out_l + done, out_r + done, k, 0);'
def instrument(source):
    assert source.count(call)==1
    # Host-only lifetime assertion: current sub-block may not touch either
    # completed output windows or the not-yet-rendered suffix.
    check = '''        float saved_l[256], saved_r[256];
        __builtin_memcpy(saved_l,out_l,sizeof saved_l);
        __builtin_memcpy(saved_r,out_r,sizeof saved_r);
CALL
        assert(!__builtin_memcmp(saved_l,out_l,done*sizeof(float)));
        assert(!__builtin_memcmp(saved_r,out_r,done*sizeof(float)));
        assert(!__builtin_memcmp(saved_l+done+k,out_l+done+k,(256-done-k)*sizeof(float)));
        assert(!__builtin_memcmp(saved_r+done+k,out_r+done+k,(256-done-k)*sizeof(float)));'''.replace('CALL',call)
    return '#include <assert.h>\n'+source.replace(call,check)
baseline.write_text(instrument(baseline_text))
candidate = OUT/'candidate.c'
candidate.write_text(instrument(candidate_text))
assert candidate_text.count('float *mono = out_l;')==1
negative = OUT/'negative.c'
negative.write_text(instrument(candidate_text.replace('float *mono = out_l;', 'float *mono = dry_l;')))
commands = []
def run(args, name):
    p = subprocess.run(list(map(str,args)), cwd=ROOT, capture_output=True, env=os.environ | {'ASAN_OPTIONS':'detect_leaks=0'})
    (OUT / (name + '.log')).write_bytes(p.stdout+p.stderr)
    commands.append({'argv':list(map(str,args)), 'exit':p.returncode,'log':name+'.log'})
    (OUT / 'commands.json').write_text(json.dumps(commands,indent=2))
    if p.returncode:
        raise RuntimeError(name + ': ' + (p.stdout+p.stderr).decode(errors='replace')[-2000:])
    return p.stdout
flags = ['-std=c99','-ffp-contract=off','-fno-fast-math','-DX0X_HOST','-DX0X_ORIGINAL_SYNTH=1',
         '-DX0X_WOBBLE=0','-DX0X_DEV_WOBBLE=0','-DORIGINAL_USE_SOFT_FMA','-DMIAMI_FX_NO_HOST_FENV',
         '-Ifirmware/src','-Ifirmware/src/app','-Ifirmware/src/dsp','-Ibuild/gen',
         '-Iexperiments/original_synth','-Iexperiments/wobble']
receipts = []
for mode, extra in [('normal',['-O2']),('sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-pie','-no-pie'])]:
    common = []
    for i, unit in enumerate(units):
        obj=OUT/f'{mode}-common-{i}.o'
        run(['cc',*flags,*extra,'-c',unit,'-o',obj],f'{mode}-common-{i}')
        common.append(obj)
    variants = [('baseline',baseline),('candidate',candidate)]
    if mode=='normal': variants.append(('negative',negative))
    for variant, src in variants:
        tag=mode+'-'+variant
        obj=OUT/(tag+'.o'); exe=OUT/tag
        run(['cc',*flags,*extra,'-Dseq_until_event=output_window_until_event',
             '-Dplat_cpu2_run=output_window_cpu2_run','-Dplat_cpu2_wait=output_window_cpu2_wait',
             '-c',src,'-o',obj],tag+'-compile')
        run(['cc',*flags,*extra,'tests/host/engine_mono_scratch_test.c',*common,obj,'-lm','-o',exe],tag+'-link')
        print(run([exe,OUT/(tag+'.pcm')],tag+'-run').decode(),end='',flush=True)
    before=(OUT/(mode+'-baseline.pcm')).read_bytes()
    after=(OUT/(mode+'-candidate.pcm')).read_bytes()
    assert before==after, mode+' PCM/meter mismatch'
    if mode=='normal':
        assert before!=(OUT/'normal-negative.pcm').read_bytes(), 'unsafe alias negative was not detected'
        print('unsafe dry_l alias detected PASS',flush=True)
    receipts.append({'mode':mode,'bytes':len(after),'sha256':hashlib.sha256(after).hexdigest(),'exact_match':True})
    print(mode+' exact PCM/meter comparison PASS',flush=True)
(OUT/'results.json').write_text(json.dumps(receipts,indent=2))
