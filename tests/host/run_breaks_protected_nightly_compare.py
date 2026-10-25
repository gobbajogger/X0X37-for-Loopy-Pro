#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exact-blob protected NIGHTLY vs PR93 BREAK renderer host work/PCM comparison.

The source projections must come from the named Git revisions. The IMA counter
is inserted in a throwaway copy of the identical GPL header for both builds.
This is producer evidence, not independent QA or microcontroller timing.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

COMMON = {
    'firmware/src/dsp/ima_adpcm.h': '9ba16e4fe9f0cb643c95a68d540042ab2581d4de',
    'firmware/src/dsp/fastmath.h': '7d2e41bf70590bafa8af0a7376bb912bf763b098',
    'firmware/src/dsp/x0x_param.h': 'f7c9bf22b0606946905790dc20497a2e4b2cb4a6',
    'firmware/src/seq/pattern.h': '9a0703418aae95b14f1b70905cfac916da38d7dd',
}
RENDERERS = {
    'nightly': {
        'firmware/src/dsp/breaks.c': '1d3d692b6788d9d1f13901cd7ab13df506cdb72f',
        'firmware/src/dsp/breaks.h': '094af6650333e844311a8806145a6177a0318225',
    },
    'pr93': {
        'firmware/src/dsp/breaks.c': '9e1e607e2ddc3028383bfb6ebc85fbc7ac6864c5',
        'firmware/src/dsp/breaks.h': '6cb71d336c2c90fdc60a9594c2356f3a4379c264',
    },
}
TEST = 'tests/host/breaks_reverse_fast_advance_test.c'
TEST_BLOB = '5e77b152253422d36ab7218669187d02318d21cb'
FUNC = 'static inline int32_t ima_decode(const uint8_t *d, uint32_t i, int32_t *pred, int32_t *idx)\n{'


def blob(data):
    return hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def invoke(args, log):
    p = subprocess.run(args, capture_output=True, text=True, timeout=150, check=False)
    log.write_text('$ ' + ' '.join(map(str, args)) + '\nexit=' + str(p.returncode) +
                   '\nSTDOUT:\n' + p.stdout + '\nSTDERR:\n' + p.stderr)
    return p.returncode, p.stdout


def rows(log):
    result = {}
    for line in log.splitlines():
        if not line.startswith('n='):
            raise RuntimeError('unexpected runner output: ' + line[:80])
        d = dict(part.split('=', 1) for part in line.split())
        key = (int(d['n']), d['direction'], int(d['rate']), int(d['trial']))
        if key in result or int(d['audible']) < 1 or d['stopped'] != '1':
            raise RuntimeError('duplicate, silent, or unstopped case: ' + str(key))
        result[key] = {k: int(d[k]) for k in ('setup', 'decodes', 'max', 'p95', 'p99', 'audible')}
        result[key]['pcm'] = d['pcm']
        if not 0 <= result[key]['p95'] <= result[key]['p99'] <= result[key]['max']:
            raise RuntimeError('invalid per-block order: ' + str(key))
    if len(result) != 140:
        raise RuntimeError('expected 140 exact workload cases, received ' + str(len(result)))
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--nightly-root', required=True, type=Path)
    p.add_argument('--pr93-root', required=True, type=Path)
    p.add_argument('--output-dir', required=True, type=Path)
    a = p.parse_args()
    if a.output_dir.exists():
        p.error('refusing to overwrite an existing output directory')
    roots = {'nightly': a.nightly_root.resolve(), 'pr93': a.pr93_root.resolve()}
    pins = {}
    for kind in roots:
        pins[kind] = {}
        for name, expected in {**COMMON, **RENDERERS[kind], **({TEST: TEST_BLOB} if kind == 'pr93' else {})}.items():
            actual = blob((roots[kind] / name).read_bytes())
            if actual != expected:
                raise RuntimeError(f'{kind} invalid Git blob {name}: {actual} != {expected}')
            pins[kind][name] = actual
    a.output_dir.mkdir(parents=True, exist_ok=False)
    found = {}
    with tempfile.TemporaryDirectory(prefix='break-nightly-pr93-') as td:
        tmp = Path(td)
        for kind, root in roots.items():
            work = tmp / kind
            for name in pins[kind]:
                to = work / name
                to.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(root / name, to)
            if kind == 'nightly':
                test_to = work / TEST
                test_to.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(roots['pr93'] / TEST, test_to)
            header = work / 'firmware/src/dsp/ima_adpcm.h'
            s = header.read_text()
            if s.count(FUNC) != 1 or s.count('#include <stdint.h>') != 1:
                raise RuntimeError('unexpected IMA header instrumentation location')
            s = s.replace('#include <stdint.h>', '#include <stdint.h>\nextern volatile uint64_t x0x_host_ima_decode_calls;')
            header.write_text(s.replace(FUNC, FUNC + '\n    ++x0x_host_ima_decode_calls;'))
            for compiler in ('gcc', 'clang'):
                tag = compiler + '-' + kind
                binary = work / tag
                command = [compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                           '-pedantic', '-ffp-contract=off', '-I' + str(work),
                           str(work / TEST), str(work / 'firmware/src/dsp/breaks.c'),
                           '-lm', '-o', str(binary)]
                ec, _ = invoke(command, a.output_dir / (tag + '-compile.log'))
                if ec:
                    raise RuntimeError(tag + ' compile exit ' + str(ec))
                trace = a.output_dir / (tag + '.pcm')
                ec, stdout = invoke([str(binary), '--check', '--trace', str(trace)],
                                    a.output_dir / (tag + '-run.log'))
                if ec:
                    raise RuntimeError(tag + ' run exit ' + str(ec))
                ec, _ = invoke([str(binary)], a.output_dir / (tag + '-negative.log'))
                if ec != 2:
                    raise RuntimeError(tag + ' invalid-argument exit ' + str(ec) + ' != 2')
                if trace.stat().st_size != 140 * 8 * 64 * 4:
                    raise RuntimeError(tag + ' unexpected trace size')
                found[tag] = {'cases': rows(stdout), 'pcm_sha256': sha(trace),
                              'trace_bytes': trace.stat().st_size}
                trace.unlink()  # no synthetic PCM distribution
    comparisons = {}
    for compiler in ('gcc', 'clang'):
        old = found[compiler + '-nightly']; new = found[compiler + '-pr93']
        if old['pcm_sha256'] != new['pcm_sha256'] or old['cases'].keys() != new['cases'].keys():
            raise RuntimeError('NIGHTLY vs PR93 whole-trace or scenario mismatch: ' + compiler)
        if any(old['cases'][k]['pcm'] != new['cases'][k]['pcm'] or
               old['cases'][k]['audible'] != new['cases'][k]['audible'] for k in old['cases']):
            raise RuntimeError('PCM/control mismatch: ' + compiler)
        stats = {}
        for direction in ('R', 'F'):
            ks = [k for k in old['cases'] if k[1] == direction]
            before = sum(old['cases'][k]['decodes'] for k in ks)
            after = sum(new['cases'][k]['decodes'] for k in ks)
            if any(new['cases'][k]['decodes'] > old['cases'][k]['decodes'] for k in ks):
                raise RuntimeError('unexpected increased decoder work: ' + direction)
            stats[direction] = {'cases': len(ks), 'baseline_decodes': before,
                                'candidate_decodes': after,
                                'reduction_percent': round(100 * (before - after) / before, 4),
                                'baseline_max_block': max(old['cases'][k]['max'] for k in ks),
                                'candidate_max_block': max(new['cases'][k]['max'] for k in ks)}
        comparisons[compiler] = {'trace_sha256': old['pcm_sha256'], 'stats': stats}
    if comparisons['gcc']['trace_sha256'] != comparisons['clang']['trace_sha256']:
        raise RuntimeError('compiler PCM mismatch')
    report = {'revision': 'performance-pr93-direct-nightly-source-compare-20261025-r19',
              'producer_only': True, 'firmware_compiler_inputs_modified': False,
              'baseline_nightly_commit': '54f2ffbc3124f7449a69067dc8215a2d91a2e675',
              'candidate_pr93_commit': '26cd60444e7b5d1ee4d62ab67f5cd85b158d2212',
              'candidate_firmware_tree': 'ee863f9cba6cbfb4e471d199ae408c7694acbac5',
              'pins': pins, 'results': comparisons,
              'source_kind': 'exact-blob C/H/dependencies + identical test source; not full repo root build',
              'known_failure': 'original unmodified PR93 fastmath.h UBSan FAIL; PR28 owns',
              'gates': {'independent_QA': 'NOT_RUN', 'Elite_Pro_target_BUILD_ONLY': 'NOT_RUN',
                        'hardware': 'NOT_RUN', 'publication': 'NOT_RUN'}}
    (a.output_dir / 'result.json').write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print('PASS scoped producer direct NIGHTLY vs PR93 PCM equivalence')
    print(json.dumps(comparisons, sort_keys=True, indent=2))


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError, subprocess.TimeoutExpired, ValueError) as ex:
        print('FAIL:', ex, file=sys.stderr)
        sys.exit(1)
