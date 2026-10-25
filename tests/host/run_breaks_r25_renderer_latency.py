#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Bounded real-breaks_render host callback latency experiment, R22 vs R23.

Run with exact pinned source projection directories; unchanged source is never
modified. Timer and decoder counters exist only in scratch benchmark fixtures.
No claim about MCU CPU cycles, XRUNs, or hardware is made by this runner.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
import run_breaks_dual_voice_decoder_compare as base

SOURCE = 'firmware/src/dsp/breaks.c'
TEST = 'tests/host/breaks_reverse_fast_advance_test.c'
BEFORE = '9e1e607e2ddc3028383bfb6ebc85fbc7ac6864c5'
AFTER = '01e0e759d2b8be3b028e0e25d4f93348a36aeb46'
RUNNER_R22 = '4a285e80482c266abc4ac818b4b0ff0910a4930c'
HEAD_PR93 = 'a7f56778cf24a1c933a8f507eaf0728a7f524614'
FIRMWARE_TREE = '75723d43185b1b7aeeaf4c50615e4381ce623a6c'
TIMER_FUNC = '''
static uint64_t host_clock_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts)) { perror("clock_gettime"); exit(3); }
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}
'''


def change_once(s, old, new):
    if s.count(old) != 1:
        raise ValueError('Expected exactly one anchor: ' + old[:90])
    return s.replace(old, new, 1)


def build_fixture(source, clock):
    s = change_once(source, '#include <inttypes.h>',
                    '#define _POSIX_C_SOURCE 200809L\n#include <time.h>\n#include <inttypes.h>')
    s = change_once(s, 'static uint32_t rnd(', TIMER_FUNC.replace('CLOCK_MONOTONIC', clock) + '\nstatic uint32_t rnd(')
    s = change_once(s, 'uint64_t per_block[BLOCKS],sum=0,max=0,overlap=0,hash=',
                    'uint64_t render_ns[BLOCKS],per_block[BLOCKS],sum=0,max=0,overlap=0,hash=')
    s = change_once(s, '        breaks_render(b,out,BLOCK);',
                    '        const uint64_t t0=host_clock_ns();\n'
                    '        breaks_render(b,out,BLOCK);\n'
                    '        render_ns[j]=host_clock_ns()-t0;')
    s = change_once(s, '    qsort(per_block,BLOCKS,sizeof(per_block[0]),cmp_u64);',
                    '    qsort(per_block,BLOCKS,sizeof(per_block[0]),cmp_u64);\n'
                    '    qsort(render_ns,BLOCKS,sizeof(render_ns[0]),cmp_u64);\n'
                    '    uint64_t render_total_ns=0;\n'
                    '    for(int k=0;k<BLOCKS;++k)render_total_ns+=render_ns[k];')
    s = change_once(s, ' overlap=%" PRIu64 " stopped=1\\n",',
                    ' overlap=%" PRIu64 " render_total_ns=%" PRIu64 " render_p50_ns=%" PRIu64 '
                    ' " render_p95_ns=%" PRIu64 " render_p99_ns=%" PRIu64 '
                    ' " render_max_ns=%" PRIu64 " stopped=1\\n",')
    s = change_once(s, 'hash,audible,overlap);',
                    'hash,audible,overlap,render_total_ns,render_ns[BLOCKS/2],'
                    'render_ns[(BLOCKS-1)*95/100],render_ns[(BLOCKS-1)*99/100],'
                    'render_ns[BLOCKS-1]);')
    return s


def run(cmd, log_path, timeout=130):
    p = subprocess.run(cmd, text=True, capture_output=True, timeout=timeout, check=False)
    log_path.write_text('command='+json.dumps(cmd)+'\nexit='+str(p.returncode)+
                        '\nstdout:\n'+p.stdout+'\nstderr:\n'+p.stderr)
    return p


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--before-root', type=Path, required=True)
    p.add_argument('--after-root', type=Path, required=True)
    p.add_argument('--output-dir', type=Path, required=True)
    p.add_argument('--repetitions', type=int, default=7)
    p.add_argument('--clock', choices=('process_cpu','monotonic'), default='process_cpu')
    a = p.parse_args()
    if not (3 <= a.repetitions <= 15):
        p.error('repetitions must be 3..15')
    if a.output_dir.exists():
        p.error('will not overwrite evidence output')
    roots = {'before': a.before_root.resolve(), 'after': a.after_root.resolve()}
    pins = {**base.COMMON, **base.RENDERERS['pr93'], TEST:base.TEST_BLOB}
    if base.blob(Path(base.__file__).read_bytes()) != RUNNER_R22:
        raise ValueError('R22 runner identity mismatch')
    for variant,root in roots.items():
        expected_code = BEFORE if variant == 'before' else AFTER
        for path,sha in {**pins,SOURCE:expected_code}.items():
            actual = base.blob((root/path).read_bytes())
            if actual != sha:raise ValueError(variant+' invalid blob '+path+': '+actual+' != '+sha)
    a.output_dir.mkdir(parents=True, exist_ok=False)
    started = time.monotonic()
    measurements = {compiler:{kind:[] for kind in roots} for compiler in ('gcc','clang')}
    with tempfile.TemporaryDirectory(prefix='x0x-r25-render-latency-') as td:
        temp=Path(td)
        for variant,root in roots.items():
            work=temp/variant
            for name in {**pins,SOURCE:None}:
                dst=work/name;dst.parent.mkdir(parents=True,exist_ok=True)
                shutil.copyfile(root/name,dst)
            fixture=(work/TEST).read_text()
            fixture=change_once(fixture,base.ORIGINAL_RATES,base.MUSICAL_RATES)
            fixture=change_once(fixture,base.ORIGINAL_SIZES,base.LONG_SIZES)
            fixture=base.live_crossfade_fixture(fixture)
            (work/TEST).write_text(build_fixture(fixture, 'CLOCK_PROCESS_CPUTIME_ID' if a.clock=='process_cpu' else 'CLOCK_MONOTONIC'))
            hdr=work/'firmware/src/dsp/ima_adpcm.h'
            s=hdr.read_text()
            s=change_once(s,'#include <stdint.h>',
                          '#include <stdint.h>\nextern volatile uint64_t x0x_host_ima_decode_calls;')
            s=change_once(s,base.FUNC,base.FUNC+'\n    ++x0x_host_ima_decode_calls;')
            hdr.write_text(s)
            for compiler in measurements:
                tag=compiler+'-'+variant
                exe=work/tag
                cmd=[compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-pedantic',
                     '-ffp-contract=off','-I'+str(work),str(work/TEST),str(work/SOURCE),'-lm',
                     '-o',str(exe)]
                z=run(cmd,a.output_dir/(tag+'.compile.log'))
                if z.returncode:raise RuntimeError(tag+' compile exit '+str(z.returncode)+' '+z.stderr[:500])
                neg=run([str(exe)],a.output_dir/(tag+'.negative.log'))
                if neg.returncode!=2:raise RuntimeError(tag+' invalid command should exit 2')
        # Paired alternating execution reduces but does not remove scheduling noise.
        for compiler in measurements:
            for trial in range(a.repetitions):
                for variant in (('before','after') if trial%2==0 else ('after','before')):
                    exe=temp/variant/(compiler+'-'+variant)
                    trace=a.output_dir/(f'{compiler}-{variant}-{trial}.pcm')
                    z=run([str(exe),'--check','--trace',str(trace)],
                          a.output_dir/(f'{compiler}-{variant}-{trial}.run.log'))
                    if z.returncode:raise RuntimeError(compiler+'/'+variant+' failed: '+z.stderr[:500])
                    expected_bytes=48*80*64*4
                    if trace.stat().st_size!=expected_bytes:raise RuntimeError('unexpected PCM trace length')
                    total=0; rev=0;fwd=0; reverse_overlap=0; keys=[]; controls=[]
                    for line in z.stdout.splitlines():
                        row=dict(x.split('=',1) for x in line.split())
                        if row.get('stopped')!='1' or int(row['audible'])<1:raise RuntimeError('missing audio or STOP')
                        key='/'.join(row[k] for k in ('n','direction','rate','trial'))
                        if key in keys:raise RuntimeError('duplicate workload case')
                        keys.append(key)
                        controls.append((key,row['pcm'],row['setup'],row['decodes'],row['overlap'],row['max']))
                        t=int(row['render_total_ns'])
                        total+=t
                        if row['direction']=='R':rev+=t;reverse_overlap+=int(row['overlap'])
                        else:fwd+=t
                        if not (int(row['render_p50_ns'])<=int(row['render_p95_ns'])<=int(row['render_p99_ns'])<=int(row['render_max_ns'])):
                            raise RuntimeError('invalid per-case latency quantiles')
                    if len(keys)!=48 or reverse_overlap==0:raise RuntimeError('missing dual voice stress cases')
                    digest=hashlib.sha256(trace.read_bytes()).hexdigest()
                    trace.unlink()
                    measurements[compiler][variant].append({'trial':trial,'render_total_ns':total,
                        'reverse_ns':rev,'forward_ns':fwd,'trace_sha256':digest,'controls':controls})
    results={}
    for compiler,groups in measurements.items():
        before,after=groups['before'],groups['after']
        for i in range(a.repetitions):
            if before[i]['trace_sha256']!=after[i]['trace_sha256']:
                raise RuntimeError(compiler+' PCM mismatch '+str(i))
            if before[i]['controls']!=after[i]['controls']:
                raise RuntimeError(compiler+' control/counter mismatch '+str(i))
        entry={'pcm_sha256':before[0]['trace_sha256'],'runs':a.repetitions,
               'reverse_median_ms':{},'forward_median_ms':{},'all_median_ms':{},
               'paired_reverse_speedup_percent':[],
               'producer_only':True}
        for key,dst in (('reverse_ns','reverse_median_ms'),('forward_ns','forward_median_ms'),('render_total_ns','all_median_ms')):
            for variant,obs in groups.items():
                entry[dst][variant]=round(statistics.median(x[key] for x in obs)/1e6,3)
        for x,y in zip(before,after):
            entry['paired_reverse_speedup_percent'].append(round((x['reverse_ns']-y['reverse_ns'])/x['reverse_ns']*100,3))
        entry['paired_reverse_speedup_median_percent']=round(statistics.median(entry['paired_reverse_speedup_percent']),3)
        results[compiler]=entry
    doc={'scope':'native host timed production-linked breaks_render callbacks; R22 vs R23 unmodified production C',
         'new_source_compiled':False,'source_pr_head':HEAD_PR93,'firmware_tree':FIRMWARE_TREE,
         'source_blobs':{'before_breaks.c':BEFORE,'after_breaks.c':AFTER,**pins},
         'environment':{'kernel':os.uname().release,'cpu':os.uname().machine,
                        'gcc':subprocess.check_output(['gcc','-dumpfullversion'],text=True).strip(),
                        'clang':subprocess.check_output(['clang','--version'],text=True).splitlines()[0]},
         'execution':{'repetitions':a.repetitions,'workload_cases_per_execution':48,'callbacks_per_execution':3840,
                      'clock':a.clock+' around breaks_render; includes symmetric instrumentation'},
         'results':results,'elapsed_seconds':round(time.monotonic()-started,3),
         'cautions':['host scheduling jitter: no fixed CPU/clock or RTOS preemption',
                     'timer/counter instrumentation affects timings but is identical in both candidates',
                     'no CPU cycle/XRUN/hardware inference','sanitizer not run: inherited fastmath.h failure PR28',
                     'no fresh Elite/Pro/controller compilation or independent QA']}
    (a.output_dir/'result.json').write_text(json.dumps(doc,indent=2,sort_keys=True)+'\n')
    print(json.dumps({k:{'paired_speedup_median_percent':v['paired_reverse_speedup_median_percent'],
                          'reverse_median_ms':v['reverse_median_ms'],'forward_median_ms':v['forward_median_ms']}
                      for k,v in results.items()},indent=2))

if __name__=='__main__':
    try:main()
    except (OSError,ValueError,RuntimeError,subprocess.TimeoutExpired) as e:
        print('FAIL:',e,file=sys.stderr);sys.exit(1)
