#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Prove checkpoint-fast-exit reachability in the real BREAK live-crossfade path.

Compare the exact R22 parent and R23 production breaks.c blobs on the same
pinned source dependencies and R22 live-key workload. Add only ephemeral host
counters to compiled copies; never modify either original source tree.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

import run_breaks_dual_voice_decoder_compare as r22

BEFORE = '9e1e607e2ddc3028383bfb6ebc85fbc7ac6864c5'
AFTER = '01e0e759d2b8be3b028e0e25d4f93348a36aeb46'
RUNNER_R22 = '4a285e80482c266abc4ac818b4b0ff0910a4930c'
CODE = 'firmware/src/dsp/breaks.c'
FIXTURE = 'tests/host/breaks_reverse_fast_advance_test.c'
ALIGNED = '''    if (i == p) {
        v->seek_pos = UINT32_MAX;
        return;
    }'''
ADD_COUNTER = '''    if (i == p) {
        ++r24_aligned_hits;
        v->seek_pos = UINT32_MAX;
        return;
    }'''


def checked(root, path, expected):
    data = (root / path).read_bytes()
    if r22.blob(data) != expected:
        raise ValueError(f'wrong source blob: {root}/{path}: {r22.blob(data)} expected {expected}')
    return data


def replace_one(s, a, b):
    if s.count(a) != 1:
        raise ValueError('source fixture lacks unique expected anchor: ' + a[:65])
    return s.replace(a, b, 1)


def run(argv, logfile, timeout=180):
    p = subprocess.run(argv, capture_output=True, text=True, timeout=timeout, check=False)
    logfile.write_text('$ ' + ' '.join(map(str, argv)) + '\nexit=' + str(p.returncode) +
                       '\nSTDOUT:\n' + p.stdout + '\nSTDERR:\n' + p.stderr)
    return p


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--before-root', required=True, type=Path)
    ap.add_argument('--after-root', required=True, type=Path)
    ap.add_argument('--output-dir', required=True, type=Path)
    a = ap.parse_args()
    if a.output_dir.exists():
        ap.error('refusing to overwrite an existing evidence directory')
    roots = {'before': a.before_root.resolve(), 'after': a.after_root.resolve()}
    pins = {**r22.COMMON, **r22.RENDERERS['pr93'], FIXTURE: r22.TEST_BLOB}
    checked(roots['before'], CODE, BEFORE)
    checked(roots['after'], CODE, AFTER)
    checked(roots['before'], FIXTURE, r22.TEST_BLOB)
    for kind in roots:
        for path, expected in pins.items():
            if path != CODE:
                checked(roots[kind], path, expected)
    if r22.blob(Path(r22.__file__).read_bytes()) != RUNNER_R22:
        raise ValueError('R22 reference helper changed')
    a.output_dir.mkdir(parents=True, exist_ok=False)
    observations = {}
    with tempfile.TemporaryDirectory(prefix='x0x-aligned-live-') as t:
        temp = Path(t)
        for kind, root in roots.items():
            dst = temp / kind
            for path in pins:
                dest = dst / path
                dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(root / path, dest)
            code = (dst / CODE).read_text()
            if kind == 'after':
                code = replace_one(code, ALIGNED, ADD_COUNTER)
                code = replace_one(code, 'static void bank_seek_reverse(',
                                   'extern volatile uint64_t r24_aligned_hits;\nstatic void bank_seek_reverse(')
                (dst / CODE).write_text(code)
            fixture = (dst / FIXTURE).read_text()
            fixture = replace_one(fixture, r22.ORIGINAL_RATES, r22.MUSICAL_RATES)
            fixture = replace_one(fixture, r22.ORIGINAL_SIZES, r22.LONG_SIZES)
            fixture = r22.live_crossfade_fixture(fixture)
            fixture = replace_one(fixture, 'volatile uint64_t x0x_host_ima_decode_calls = 0;',
                                  'volatile uint64_t x0x_host_ima_decode_calls = 0;\nvolatile uint64_t r24_aligned_hits = 0;')
            fixture = replace_one(fixture, '    x0x_host_ima_decode_calls=0;',
                                  '    x0x_host_ima_decode_calls=0;\n    r24_aligned_hits=0;')
            fixture = replace_one(fixture, ' overlap=%" PRIu64 " stopped=1\\n",',
                                  ' overlap=%" PRIu64 " aligned=%" PRIu64 " stopped=1\\n",')
            fixture = replace_one(fixture, 'hash,audible,overlap);',
                                  'hash,audible,overlap,r24_aligned_hits);')
            (dst / FIXTURE).write_text(fixture)
            hdr = dst / 'firmware/src/dsp/ima_adpcm.h'
            orig = hdr.read_text()
            if orig.count(r22.FUNC) != 1 or orig.count('#include <stdint.h>') != 1:
                raise ValueError('IMA decoder instrumentation anchor mismatch')
            hdr.write_text(orig.replace('#include <stdint.h>',
                         '#include <stdint.h>\nextern volatile uint64_t x0x_host_ima_decode_calls;')
                         .replace(r22.FUNC, r22.FUNC + '\n    ++x0x_host_ima_decode_calls;'))
            for compiler in ('gcc', 'clang'):
                tag = f'{compiler}-{kind}'
                exe = dst / tag
                cmd = [compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror','-pedantic',
                       '-ffp-contract=off','-I'+str(dst),str(dst/FIXTURE),str(dst/CODE),
                       '-lm','-o',str(exe)]
                p = run(cmd, a.output_dir / (tag+'.compile.log'))
                if p.returncode:
                    raise RuntimeError(f'{tag} compiler failed: {p.returncode}: {p.stderr[:500]}')
                trace = a.output_dir / (tag+'.pcm')
                p = run([str(exe),'--check','--trace',str(trace)], a.output_dir / (tag+'.run.log'))
                if p.returncode:
                    raise RuntimeError(f'{tag} render failed: {p.returncode}: {p.stderr[:500]}')
                neg = run([str(exe)], a.output_dir / (tag+'.negative.log'))
                if neg.returncode != 2:
                    raise RuntimeError(f'{tag} expected invalid invocation exit 2, got {neg.returncode}')
                if trace.stat().st_size != 48*80*64*4:
                    raise RuntimeError(f'{tag} incorrect PCM length')
                rows = {}
                for line in p.stdout.splitlines():
                    parts = dict(kv.split('=',1) for kv in line.split())
                    key = tuple(parts[k] for k in ('n','direction','rate','trial'))
                    if key in rows or not all(k in parts for k in ('aligned','overlap','pcm','stopped')):
                        raise RuntimeError(f'{tag} missing/duplicate coverage event')
                    rows[key] = parts
                if len(rows) != 48:
                    raise RuntimeError(f'{tag} expected 48 real-path cases, got {len(rows)}')
                observations[tag] = {'pcm_sha256': hashlib.sha256(trace.read_bytes()).hexdigest(),
                                     'cases': rows, 'negative_exit': neg.returncode}
                trace.unlink()
    summary = {}
    for compiler in ('gcc','clang'):
        before = observations[compiler+'-before']
        after = observations[compiler+'-after']
        if before['pcm_sha256'] != after['pcm_sha256']:
            raise RuntimeError(f'{compiler} changed observable PCM')
        hits = 0
        by_case = {}
        for key, old in before['cases'].items():
            new = after['cases'].get(key)
            if not new:
                raise RuntimeError(f'{compiler} missing case {key}')
            for field in ('pcm','audible','overlap','decodes','setup','stopped','max','p95','p99'):
                if old[field] != new[field]:
                    raise RuntimeError(f'{compiler} changed {field} in real callback case {key}')
            n = int(new['aligned'])
            if int(old['aligned']) != 0:
                raise RuntimeError(f'{compiler} R22 unexpectedly has new branch')
            if key[1] == 'F' and n:
                raise RuntimeError(f'{compiler} forward playback used reverse fast exit')
            hits += n
            by_case['/'.join(key)] = n
        if hits == 0 or not any(v>0 and '/R/' in k and k.endswith('/1') for k,v in by_case.items()):
            raise RuntimeError(f'{compiler} R23 fast exit not reached during reverse live-pad overlap')
        summary[compiler] = {'real_path_aligned_exit_hits':hits,'pcm_sha256':after['pcm_sha256'],
                             'cases':len(after['cases']), 'reverse_live_overlap_cases_with_hits':
                             sum(v>0 and '/R/' in k and k.endswith('/1') for k,v in by_case.items()),
                             'per_case_hits':by_case}
    if summary['gcc']['pcm_sha256'] != summary['clang']['pcm_sha256']:
        raise RuntimeError('compiler PCM differs')
    report = {'scope':'producer host-only test-only counters on copied source; NOT independent QA or MCU timing',
              'before_breaks_blob':BEFORE,'after_breaks_blob':AFTER,'source_pins':pins,
              'r22_runner_blob':RUNNER_R22,'results':summary,
              'not_run':['Elite/Pro/controller build','independent QA','target timing/XRUN','device','installer'],
              'inherited_failure':'Original fastmath.h:57 UBSan negative shift; PR28 owns'}
    (a.output_dir/'result.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print('PASS: true R23 aligned-checkpoint fast exits observed in live two-voice reverse playback')
    for compiler, x in summary.items():
        print(compiler,x['real_path_aligned_exit_hits'],x['reverse_live_overlap_cases_with_hits'],x['pcm_sha256'])


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, AssertionError, RuntimeError, subprocess.TimeoutExpired) as ex:
        print('FAIL:',ex,file=sys.stderr)
        sys.exit(1)
