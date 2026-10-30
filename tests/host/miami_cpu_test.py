#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Local Miami bit-exact regression and CPU benchmark; no plugins or devices.

--compare-ref exports public source from a local Git commit into the ignored
output directory, then overlays only the pinned external headers. Binary PCM
and state traces remain ignored. No network, checkout, staging or push occurs.
"""
import argparse
import ast
import filecmp
import hashlib
import io
import json
import os
from pathlib import Path
import platform
import shlex
import shutil
import statistics
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = Path('experiments/original_synth/EXTERNAL-INPUTS.json')


def verify(source):
    subprocess.run(['python3', str(ROOT/'tools/check_original_inputs.py'),
                    '--root', str(source), '--scope', 'all'], check=True)


def build(source, out, args):
    out.mkdir(parents=True, exist_ok=True)
    verify(source)
    tree = ast.parse((source/'tools/build.py').read_text())
    units = next(ast.literal_eval(n.value) for n in tree.body
                 if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name)
                 and t.id == 'ORIGINAL_SYNTH_UNITS' for t in n.targets))
    flags = ['-std=c99', '-Os', '-fno-builtin', '-fno-fast-math', '-ffp-contract=off',
             '-frounding-math', '-DMIAMI_FX_NO_HOST_FENV',
             '-ftrivial-auto-var-init=zero', '-Wall', '-Wextra', '-Werror',
             '-Wno-misleading-indentation', '-Wno-array-parameter',
             '-I'+str(source/'firmware/src/dsp'),
             '-I'+str(source/'experiments/original_synth'),
             '-I'+str(source/'experiments/wobble')]
    if not args.native_fma:
        flags += ['-DORIGINAL_USE_SOFT_FMA']
    if args.sanitize:
        flags += ['-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    if args.profile:
        flags += ['-g', '-fno-omit-frame-pointer']
    if args.idle_proxy:
        flags += ['-DMIAMI_CPU_COVERAGE', '-fsanitize-coverage=trace-pc-guard,no-prune']
    cc = shlex.split(os.environ.get('CC', 'cc'))
    command = [*cc, *flags, str(ROOT/'tests/host/miami_cpu_test.c'),
               *(str(source/u) for u in units),
               str(source/'firmware/src/dsp/original_miami_backend.c'), '-lm', '-o', str(out/'runner')]
    (out/'command.txt').write_text(shlex.join(command)+'\n')
    subprocess.run(command, check=True)
    return cc, flags


def export_reference(ref, out):
    sha = subprocess.check_output(['git', 'rev-parse', '--verify', ref+'^{commit}'], cwd=ROOT, text=True).strip()
    out.mkdir()
    archive = subprocess.check_output(['git', 'archive', sha, 'experiments/original_synth',
                                      'experiments/wobble', 'firmware/src/dsp', 'tools/build.py'], cwd=ROOT)
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        # These source scopes contain regular files and directories only. Copy
        # them explicitly, including on Python versions without tar's data
        # filter; never follow archive links or paths outside the fresh tree.
        for member in tar:
            path = Path(member.name)
            if path.is_absolute() or '..' in path.parts:
                raise SystemExit('Unsafe reference path: '+member.name)
            destination = out/path
            if member.isdir():
                destination.mkdir(parents=True, exist_ok=True)
            elif member.isfile():
                destination.parent.mkdir(parents=True, exist_ok=True)
                with tar.extractfile(member) as src, destination.open('wb') as dst:
                    shutil.copyfileobj(src, dst)
            else:
                raise SystemExit('Unsupported reference member: '+member.name)
    # Private bytes are read only after verifying the manifest and ignored
    # paths. The overlay itself is below build/, which must also be ignored.
    for item in json.loads((out/MANIFEST).read_text())['excluded_data_inputs']:
        path = item['path']
        subprocess.run(['git', 'check-ignore', '--quiet', path], cwd=ROOT, check=True)
        data = (ROOT/path).read_bytes()
        if len(data) != item['bytes'] or hashlib.sha256(data).hexdigest() != item['sha256']:
            raise SystemExit('Reference input mismatch: '+path)
        (out/path).parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT/path, out/path)
    verify(out)
    return sha


def unit_tests(cc, flags, out):
    groups = {
        'zero': [],
        'idle_clock': [],
        'pow2_fma': [],  # selected active MG/VCF bit-exact math regression
        'table_interpolation': ['experiments/original_synth/fused_reference.c'],
        'bypass': ['experiments/original_synth/miami_fx_chain.c',
                   'experiments/original_synth/fixed_fx.c',
                   'experiments/wobble/miami_decimator_adapter.c',
                   'experiments/wobble/miami_decimator_component.c',
                   'experiments/wobble/miami_fx_conversion.c',
                   'experiments/original_synth/fused_reference.c'],
        'interpolation': ['experiments/original_synth/miami_ms20.c',
                          'experiments/original_synth/fused_reference.c'],
    }
    for name, sources in groups.items():
        exe = out/(name+'-test')
        subprocess.run([*cc, *flags, str(ROOT/f'tests/host/miami_{name}_test.c'),
                        *(str(ROOT/s) for s in sources), '-lm', '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, default=ROOT)
    p.add_argument('--out', type=Path, required=True, help='fresh directory under build/')
    p.add_argument('--sanitize', action='store_true')
    p.add_argument('--native-fma', action='store_true', help='also qualify the host hardware-FMA route')
    p.add_argument('--profile', action='store_true', help='retain symbols and frame pointers')
    p.add_argument('--idle-proxy', action='store_true',
                   help='Clang instrumented edge counts, not CPU instructions; compare-ref required')
    p.add_argument('--compare-ref', help='local immutable baseline commit')
    p.add_argument('--bench-rounds', type=int, default=0)
    p.add_argument('--repeats', type=int, default=10)
    args = p.parse_args()
    source, out = args.source.resolve(), args.out.resolve()
    if not out.is_relative_to(ROOT/'build') or out.exists():
        p.error('--out must be a fresh directory under this worktree build/')
    if args.bench_rounds < 0 or args.repeats < 1:
        p.error('benchmark rounds must be nonnegative and repeats positive')
    if args.idle_proxy and not args.compare_ref:
        p.error('--idle-proxy requires --compare-ref')
    subprocess.run(['git', 'check-ignore', '--quiet', str(out)], cwd=ROOT, check=True)
    out.mkdir(parents=True)
    verify(ROOT)
    cc, flags = build(source, out/'candidate', args)
    if args.idle_proxy:
        ref = export_reference(args.compare_ref, out/'reference-source')
        build(out/'reference-source', out/'reference', args)
        result = {'reference': ref, 'metric': 'Clang instrumented edges per 256-frame idle block',
                  'not_cpu_instructions': True, 'blocks': 1024, 'warmup_blocks': 64,
                  'bpm': 217.5, 'compiler': subprocess.check_output([*cc, '--version'], text=True)}
        for side in ('reference', 'candidate'):
            output = subprocess.check_output([str(out/side/'runner'), 'idle-count'], text=True)
            (out/side/'idle-count.txt').write_text(output)
            result[side+'_edges_per_block'] = [float(line.split()[-1]) for line in output.splitlines()]
        (out/'result.json').write_text(json.dumps(result, indent=2)+'\n')
        print(json.dumps(result, indent=2))
        return
    if source == ROOT:
        unit_tests(cc, flags, out/'candidate')
    subprocess.run([str(out/'candidate/runner'), 'proof', str(out/'candidate/trace.bin')], check=True)
    subprocess.run([str(out/'candidate/runner'), 'idle-proof', str(out/'candidate/idle-trace.bin')], check=True)
    subprocess.run([str(out/'candidate/runner'), 'idle-range-proof', str(out/'candidate/idle-range-trace.bin')], check=True)
    if not args.compare_ref:
        print('Candidate PCM/state trace:', out/'candidate/trace.bin')
        return
    ref = export_reference(args.compare_ref, out/'reference-source')
    build(out/'reference-source', out/'reference', args)
    result = {'reference': ref, 'manifest_sha256': hashlib.sha256((ROOT/MANIFEST).read_bytes()).hexdigest(),
              'platform': platform.platform(), 'compiler': subprocess.check_output([*cc, '--version'], text=True),
              'sanitize': args.sanitize, 'native_fma': args.native_fma, 'profile': args.profile}
    subprocess.run([str(out/'reference/runner'), 'proof', str(out/'reference/trace.bin')], check=True)
    subprocess.run([str(out/'reference/runner'), 'idle-proof', str(out/'reference/idle-trace.bin')], check=True)
    subprocess.run([str(out/'reference/runner'), 'idle-range-proof', str(out/'reference/idle-range-trace.bin')], check=True)
    if not filecmp.cmp(out/'reference/trace.bin', out/'candidate/trace.bin', shallow=False):
        raise SystemExit('FAIL: Miami PCM/state traces differ')
    result['trace_sha256'] = hashlib.sha256((out/'candidate/trace.bin').read_bytes()).hexdigest()
    result['trace_bytes'] = (out/'candidate/trace.bin').stat().st_size
    result['exact'] = True
    if not filecmp.cmp(out/'reference/idle-trace.bin', out/'candidate/idle-trace.bin', shallow=False):
        raise SystemExit('FAIL: Miami long-idle/note PCM/state traces differ')
    result['idle_trace_sha256'] = hashlib.sha256((out/'candidate/idle-trace.bin').read_bytes()).hexdigest()
    result['idle_trace_bytes'] = (out/'candidate/idle-trace.bin').stat().st_size
    if not filecmp.cmp(out/'reference/idle-range-trace.bin', out/'candidate/idle-range-trace.bin', shallow=False):
        raise SystemExit('FAIL: Miami exhaustive idle lengths 1..2000/note traces differ')
    result['idle_range_trace_sha256'] = hashlib.sha256((out/'candidate/idle-range-trace.bin').read_bytes()).hexdigest()
    result['idle_range_trace_bytes'] = (out/'candidate/idle-range-trace.bin').stat().st_size
    print('PASS byte-exact baseline/candidate PCM, workspace and state')
    times = {'reference': [], 'candidate': []}
    for r in range(args.bench_rounds):
        for side in (['reference', 'candidate'] if r % 2 == 0 else ['candidate', 'reference']):
            output = subprocess.check_output([str(out/side/'runner'), 'bench', str(args.repeats)], text=True)
            (out/side/f'timing-{r}.txt').write_text(output)
            times[side].append([float(line.split()[-1]) for line in output.splitlines()])
    if args.bench_rounds:
        result['benchmark'] = {'rounds': args.bench_rounds, 'repeats': args.repeats,
                               'ns_per_frame_by_factory': times}
        medians = {s: statistics.median(statistics.mean(t) for t in ts) for s, ts in times.items()}
        result['benchmark']['median_round_mean_ns_per_frame'] = medians
        result['benchmark']['reduction_percent'] = 100*(1-medians['candidate']/medians['reference'])
        print(json.dumps({k: v for k, v in result['benchmark'].items() if k != 'ns_per_frame_by_factory'}, indent=2))
    (out/'result.json').write_text(json.dumps(result, indent=2)+'\n')


if __name__ == '__main__':
    main()
