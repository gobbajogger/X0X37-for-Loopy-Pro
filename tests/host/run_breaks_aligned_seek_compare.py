#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Producer-side real BREAK regression for a checkpoint-aligned reverse-seek fast exit.

Reconstructs the exact unmodified PR93 parent DSP source by removing ONLY the
reviewed branch body, validating its Git blob against the actual remote parent.
The paired sources compile against identical verified dependencies and existing
production-linked host tests. Neither the test nor its timing certifies MCU ISR.
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

SOURCE = 'firmware/src/dsp/breaks.c'
PARENT_BLOB = '9e1e607e2ddc3028383bfb6ebc85fbc7ac6864c5'
CHANGED_BLOB = '01e0e759d2b8be3b028e0e25d4f93348a36aeb46'
PINS = {
    'firmware/src/dsp/breaks.h': '6cb71d336c2c90fdc60a9594c2356f3a4379c264',
    'firmware/src/dsp/ima_adpcm.h': '9ba16e4fe9f0cb643c95a68d540042ab2581d4de',
    'firmware/src/dsp/fastmath.h': '7d2e41bf70590bafa8af0a7376bb912bf763b098',
    'firmware/src/dsp/x0x_param.h': 'f7c9bf22b0606946905790dc20497a2e4b2cb4a6',
    'firmware/src/seq/pattern.h': '9a0703418aae95b14f1b70905cfac916da38d7dd',
    'tests/host/breaks_reverse_fast_advance_test.c': '5e77b152253422d36ab7218669187d02318d21cb',
    'tests/host/breaks_live_stress_test.c': '1df9b9920f87ba08d05897a9e4684f85c85c90a4',
}
BENCH = 'tests/host/breaks_aligned_reverse_seek_benchmark.c'
PR28_HEADER = 'a5c26c95cad0d91a548a8db073f59777e36d4e86'
INSERTED = '''    /* rev_fetch passes a BRK_RCH-aligned base. If it is also an exact
     * checkpoint, no decoding or adjacent-state lookup is necessary. */
    if (i == p) {
        v->seek_pos = UINT32_MAX;
        return;
    }
'''


def blob(b):
    return hashlib.sha1(b'blob ' + str(len(b)).encode() + b'\0' + b).hexdigest()


def sha(b):
    return hashlib.sha256(b).hexdigest()


def run(argv, log, timeout=90, env=None):
    p = subprocess.run(argv, capture_output=True, text=True, timeout=timeout,
                       check=False, env=env)
    log.write_text('$ ' + ' '.join(map(str, argv)) + '\nexit=' + str(p.returncode)
                   + '\nSTDOUT:\n' + p.stdout + '\nSTDERR:\n' + p.stderr)
    return p


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source-root', required=True, type=Path)
    ap.add_argument('--output-dir', required=True, type=Path)
    ap.add_argument('--peer-fastmath-file', type=Path,
                    help='Exact PR28 repair, used in throwaway ASan/UBSan projections only')
    ap.add_argument('--skip-benchmark', action='store_true')
    a = ap.parse_args()
    root = a.source_root.resolve()
    out = a.output_dir.resolve()
    if out.exists():
        ap.error('refusing to replace existing output or evidence')
    src = (root / SOURCE).read_bytes()
    if blob(src) != CHANGED_BLOB:
        raise ValueError(f'wrong current source blob {blob(src)}, expected {CHANGED_BLOB}')
    changed = src.decode('utf-8')
    if changed.count(INSERTED) != 1:
        raise ValueError('unexpected checkpoint-fast-path edit')
    parent = changed.replace(INSERTED, '', 1).encode('utf-8')
    if blob(parent) != PARENT_BLOB:
        raise ValueError('old source reconstruction does not match actual Git parent')
    for p, expected in PINS.items():
        got = blob((root / p).read_bytes())
        if got != expected:
            raise ValueError(f'wrong input {p}: {got} != {expected}')
    bench_bytes = (root / BENCH).read_bytes()
    if b'bank_seek_reverse(v,&b->bank[0],p,' not in bench_bytes:
        raise ValueError('benchmark does not call the real BREAK seek helper')
    peer = None
    if a.peer_fastmath_file:
        peer = a.peer_fastmath_file.read_bytes()
        if blob(peer) != PR28_HEADER:
            raise ValueError('the sanitizer projection is not the exact PR28 repair')
    out.mkdir(parents=True, exist_ok=False)
    result = {'scope': 'PRODUCER HOST TEST; NOT independent QA, MCU deadline, model build, or device PASS',
              'before_source_blob': PARENT_BLOB, 'after_source_blob': CHANGED_BLOB,
              'unchanged_blobs': PINS, 'microbench_blob': blob(bench_bytes),
              'peer_PR28_sanitizer_projection': PR28_HEADER if peer is not None else 'NOT_RUN',
              'tests': [], 'microbench': [], 'outstanding': {
                 'owner_Elite_Pro_controller_compilation': 'NOT_RUN', 'independent_full_root_QA': 'NOT_RUN',
                 'original_source_UBSan': 'INHERITED FAIL (fastmath.h:57, existing PR28 owner)',
                 'device_ISR': 'NOT_RUN', 'hardware': 'NOT_RUN'}}
    with tempfile.TemporaryDirectory(prefix='x0x-aligned-seek-') as t:
        tmp = Path(t)
        variants = {}
        for kind, source in (('before', parent), ('after', src)):
            folder = tmp / kind
            variants[kind] = folder
            for path in PINS:
                dest = folder / path
                dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(root / path, dest)
            dest = folder / SOURCE
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(source)
            dest = folder / BENCH
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(bench_bytes)
        fixtures = [('fast-reverse', 'tests/host/breaks_reverse_fast_advance_test.c',
                     ['--check', '--trace'], 5*2*7*2, 5*2*7*2*8*64*4),
                    ('live-stress', 'tests/host/breaks_live_stress_test.c',
                     [], 16, 16*320*64*4)]
        for compiler in ('gcc', 'clang'):
            for flags, label in ((['-O2'], 'opt'),
                                 (['-O1', '-g', '-fno-omit-frame-pointer',
                                   '-fsanitize=address,undefined'], 'san_projection') if peer else ([], 'not_run')):
                if not flags:
                    continue
                rows = {}
                for kind in ('before', 'after'):
                    folder = variants[kind]
                    if label == 'san_projection':
                        (folder / 'firmware/src/dsp/fastmath.h').write_bytes(peer)
                    for label_test, fixture, args, cases, pcm_length in fixtures:
                        exe = folder / (compiler+'-'+label+'-'+label_test)
                        base_cmd = [compiler, '-std=c11', '-Wall', '-Wextra', '-Werror',
                                    '-pedantic', '-ffp-contract=off', '-I'+str(folder)] + flags
                        cmd = base_cmd + [str(folder / fixture), str(folder / SOURCE), '-lm', '-o', str(exe)]
                        prefix = f'{compiler}-{label}-{label_test}-{kind}'
                        comp_log = out / (prefix+'.compile.log')
                        comp = run(cmd, comp_log)
                        if comp.returncode:
                            raise RuntimeError(f'{prefix} compilation failed: {comp.returncode}')
                        pcm = folder / (prefix+'.pcm')
                        cmd = [str(exe)] + args + [str(pcm)]
                        env = os.environ.copy()
                        if label == 'san_projection':
                            env.update(ASAN_OPTIONS='detect_leaks=1:abort_on_error=1',
                                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
                        r = run(cmd, out / (prefix+'.run.log'), env=env)
                        if r.returncode:
                            raise RuntimeError(f'{prefix} execution failed: {r.returncode}')
                        if pcm.stat().st_size != pcm_length:
                            raise RuntimeError(f'{prefix} trace length mismatch: {pcm.stat().st_size}')
                        lines = [s for s in r.stdout.splitlines() if s.startswith('n=')]
                        if len(lines) != cases or len(set(lines)) != cases:
                            raise RuntimeError(f'{prefix} scenario count mismatch')
                        if any('silent=0' in s or 'stopped=0' in s or 'audible=0' in s for s in lines):
                            raise RuntimeError(f'{prefix} silence/audibility control failed')
                        bad = run([str(exe)], out / (prefix+'.negative.log'), env=env)
                        if bad.returncode != 2:
                            raise RuntimeError(f'{prefix} invalid argument not rejected')
                        rows[(kind,label_test)] = (sha(pcm.read_bytes()), r.stdout)
                        result['tests'].append({'config':prefix,'compile_exit':0,'render_exit':0,
                            'negative_exit':2, 'scenarios':cases, 'pcm_bytes':pcm_length,
                            'pcm_sha256':rows[(kind,label_test)][0],
                            'compile_log_sha256':sha(comp_log.read_bytes())})
                        pcm.unlink()
                    # Restore original for optimized modes; never mutate tracked root.
                    if label == 'san_projection':
                        shutil.copyfile(root / 'firmware/src/dsp/fastmath.h',
                                        folder / 'firmware/src/dsp/fastmath.h')
                for label_test, *_ in fixtures:
                    if rows[('before',label_test)] != rows[('after',label_test)]:
                        raise RuntimeError(f'{compiler} {label} not bit-identical PCM and controls: {label_test}')
        if not a.skip_benchmark:
            for compiler in ('gcc','clang'):
                for kind in ('before','after'):
                    folder = variants[kind]
                    cmd = [compiler, '-std=c11','-O2','-Wall','-Wextra','-Werror',
                           '-pedantic','-ffp-contract=off','-I'+str(folder),
                           str(folder / BENCH), '-lm','-o',str(folder/(compiler+'-bench'))]
                    r = run(cmd, out / (compiler+'-bench-'+kind+'.compile.log'))
                    if r.returncode:
                        raise RuntimeError(f'microbench failed compile: {compiler} {kind}')
                for length in (65535,176400):
                    times = {'before': [], 'after': []}
                    checksums = {'before': [], 'after': []}
                    for trial in range(9):
                        kinds = ('before','after') if trial%2==0 else ('after','before')
                        for kind in kinds:
                            exe = variants[kind] / (compiler+'-bench')
                            start = time.perf_counter_ns()
                            p = subprocess.run([str(exe), str(length)], capture_output=True, text=True,
                                               check=False, timeout=90)
                            elapsed = (time.perf_counter_ns()-start)/1e6
                            if p.returncode or not p.stdout.startswith(f'length={length} '):
                                raise RuntimeError('benchmark failed actual seek path')
                            times[kind].append(round(elapsed,6))
                            checksums[kind].append(p.stdout.strip())
                        if checksums['before'][-1] != checksums['after'][-1]:
                            raise RuntimeError('real seek helper state/algorithm mismatch')
                    bm = statistics.median(times['before']); am = statistics.median(times['after'])
                    result['microbench'].append({'compiler':compiler, 'length':length,
                        'calls_per_trial':25000000, 'median_before_ms':round(bm,4),
                        'median_after_ms':round(am,4), 'change_percent':round(100*(am-bm)/bm,3),
                        'raw_elapsed_ms':times, 'exact_state_checksum':checksums['before'][0],
                        'note':'host process elapsed; noisy; not complete callback or target-cycle speed'})
    result['disposition'] = 'PRODUCER HOST PCM/controls matched; target budgets and independent QA pending'
    (out / 'result.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print('PASS: source-blob verified, GCC/Clang renderer PCM and NoteOff/STOP parity')
    for b in result['microbench']:
        print(b['compiler'],b['length'],b['change_percent'],'percent elapsed')

if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as ex:
        print('FAIL:',ex,file=sys.stderr)
        sys.exit(1)
