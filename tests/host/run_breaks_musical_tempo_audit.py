#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Real BREAK host test at tempo-derived rates, across pinned source revisions.

Requires two Git-exact source roots supplied by the tester. No target compiler,
private sample, firmware, physical-device or independent-QA claim is implied.
"""
import argparse
import hashlib
import importlib.util
import json
import os
import subprocess
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
legacy = HERE / 'run_breaks_decoder_work_audit.py'
spec = importlib.util.spec_from_file_location('x0x_decoder_work_reuse', legacy)
assert spec and spec.loader
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
NCASES=2*3*3*2*2*2
EXPECTED_BYTES=NCASES*64*64*4


def invoke(cmd,log,env=None):
    x=subprocess.run(cmd,text=True,capture_output=True,env=env)
    log.write_text(x.stdout+('\nSTDERR:\n'+x.stderr if x.stderr else ''))
    return {'command':cmd,'exit':x.returncode,'log':log.name,
            'log_sha256':hashlib.sha256(log.read_bytes()).hexdigest()}


def parse(log):
    rows={}
    for line in log.read_text().splitlines():
        if not line.startswith('n='):
            raise ValueError('unexpected non-case line')
        fields=dict(s.split('=',1) for s in line.split())
        key=tuple(fields[s] for s in ('n','bars','bpm','semis','dir','trial'))
        if key in rows:raise ValueError('duplicate case')
        if fields['stopped']!='1' or int(fields['audible'])<=0:raise ValueError('silent/stopped failure')
        if not (0<=int(fields['p95'])<=int(fields['p99'])<=int(fields['max'])):
            raise ValueError('invalid work percentiles')
        rows[key]=fields
    if len(rows)!=NCASES:raise ValueError(f'incomplete cases {len(rows)}/{NCASES}')
    return rows


def compare(a,b):
    if a.keys()!=b.keys():raise ValueError('different scenarios')
    for k in a:
        for signal in ('pcm','audible','stopped'):
            if a[k][signal]!=b[k][signal]:raise ValueError(f'PCM/control mismatch {k}: {signal}')
    records=[]
    for k in sorted(a):
        n,bars,bpm,semi,direction,trial=k
        x,y=a[k],b[k]
        records.append({'n':int(n),'bars':int(bars),'bpm_equivalent':int(bpm),
                        'pitch_semitones':int(semi),'direction':direction,'trial':int(trial),
                        'baseline_decodes':int(x['decodes']),'candidate_decodes':int(y['decodes']),
                        'baseline_max_per_64_frames':int(x['max']),
                        'candidate_max_per_64_frames':int(y['max']),
                        'pcm_FNV64':x['pcm']})
    return records


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--baseline-root',type=Path,required=True)
    p.add_argument('--candidate-root',type=Path,required=True)
    p.add_argument('--output-dir',type=Path,required=True)
    p.add_argument('--compiler',choices=['gcc','clang','both'],default='both')
    p.add_argument('--sanitizer',action='store_true')
    args=p.parse_args()
    if args.baseline_root.resolve()==args.candidate_root.resolve():p.error('different sources required')
    if args.output_dir.exists():p.error('refuse to overwrite evidence')
    args.output_dir.mkdir(parents=True)
    harness=HERE / 'breaks_musical_tempo_work_audit.c'
    if not harness.is_file() or not legacy.is_file():raise FileNotFoundError('host test source missing')
    out={'status':'SCOPED_SOURCE_EXACT_HOST_ONLY','firmware_compiled':False,'device_tested':False,
         'cases_per_configuration':NCASES,'tests':[],'sources':{},'matrix':{},
         'tempo_formula':'samples_per_16th=44100*60/(4*bpm), bpm in 60,120,300',
         'runtime_scope':'complete production breaks.c with one host-only volatile decoder observer'}
    with tempfile.TemporaryDirectory(prefix='x0x-musical-tempo-') as tmp:
        td=Path(tmp)
        for name,root in [('baseline-nightly',args.baseline_root),('candidate',args.candidate_root)]:
            out['sources'][name]=mod.take_source(name,root,td/name)
        for cc in (('gcc','clang') if args.compiler=='both' else (args.compiler,)):
            for name in ('baseline-nightly','candidate'):
                src=td/name
                exe=td/f'{cc}-{name}'
                cmd=[cc,'-std=c11','-Wall','-Wextra','-Werror','-pedantic',
                     '-ffp-contract=off','-I'+str(src)]
                if args.sanitizer:
                    cmd+=['-O1','-g','-fno-omit-frame-pointer','-fsanitize=address,undefined']
                else:cmd+=['-O2']
                cmd+=[str(harness),str(src/'firmware/src/dsp/breaks.c'),'-o',str(exe)]
                outcome=invoke(cmd,args.output_dir/f'{cc}-{name}-compile.log')
                out['tests'].append(outcome)
                if outcome['exit']:raise RuntimeError('compile failed')
                trace=args.output_dir/f'{cc}-{name}-pcm.bin'
                cmd=[str(exe),'--trace',str(trace)]
                env=os.environ.copy()
                env.update(ASAN_OPTIONS='detect_leaks=1:abort_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
                outcome=invoke(cmd,args.output_dir/f'{cc}-{name}-run.log',env)
                out['tests'].append(outcome)
                if outcome['exit']:raise RuntimeError('render failed')
                if trace.stat().st_size!=EXPECTED_BYTES:raise ValueError('wrong PCM trace size')
                out['matrix'].setdefault(cc,{})[name]={
                    'rows':parse(args.output_dir/f'{cc}-{name}-run.log'),
                    'trace_sha256':hashlib.sha256(trace.read_bytes()).hexdigest(),
                    'trace_bytes':trace.stat().st_size}
                invalid=invoke([str(exe),'--invalid'],args.output_dir/f'{cc}-{name}-invalid.log')
                out['tests'].append({**invalid,'expected_exit':2})
                if invalid['exit']!=2:raise ValueError('invalid selector did not fail closed')
            a=out['matrix'][cc]['baseline-nightly'];b=out['matrix'][cc]['candidate']
            if a['trace_sha256']!=b['trace_sha256']:raise ValueError('full trace mismatch '+cc)
            comparison=compare(a['rows'],b['rows'])
            corrupt=dict(b['rows'])
            first=next(iter(corrupt))
            corrupt[first]={**corrupt[first],'pcm':'ffffffffffffffff'}
            try:compare(a['rows'],corrupt)
            except ValueError:pass
            else:raise ValueError('comparison accepted deliberately corrupted PCM')
            out['matrix'][cc]['scenarios']=comparison
            for name in ('baseline-nightly','candidate'):
                out['matrix'][cc][name].pop('rows')
        (args.output_dir/'summary.json').write_text(json.dumps(out,indent=2,sort_keys=True)+'\n')
    return 0

if __name__=='__main__':raise SystemExit(main())
