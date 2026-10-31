#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host-only peak probe; never builds firmware, flashes, or changes Git refs.

python3 tests/host/synth_peak_test.py --out build/peak-check \
    --reference-source /path/to/exact-base
The reference must contain the source closure at the intended base revision.
Raw PCM and timing stay under --out. A reference comparison requires exact bytes.
"""
import argparse
import ast
import csv
import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def build(source, out, exact, sanitize, count_width=False):
    subprocess.run(['python3', str(source/'tools/check_original_inputs.py'),
                    '--root', str(source), '--scope', 'active'], check=True)
    names = {'ORIGINAL_SYNTH_UNITS', 'ORIGINAL_SYNTH_BACKENDS', 'ORIGINAL_SYNTH_FLAGS'}
    values = {}
    for node in ast.parse((source/'tools/build.py').read_text()).body:
        if isinstance(node, ast.Assign) and isinstance(node.targets[0], ast.Name):
            if node.targets[0].id in names:
                values[node.targets[0].id] = ast.literal_eval(node.value)
    flags = values['ORIGINAL_SYNTH_FLAGS']
    if exact:
        flags = [f for f in flags if f != '-DX0X_FAST_FMA']
    units = values['ORIGINAL_SYNTH_UNITS'] + values['ORIGINAL_SYNTH_BACKENDS']
    units += ['firmware/src/dsp/x0x_param.c']
    command = [os.environ.get('CC', 'cc'), '-std=c99', '-O2',
               '-D_POSIX_C_SOURCE=200809L', '-DX0X_HOST', '-DBOARD_SMK37=1',
               '-DX0X_ORIGINAL_SYNTH=1', *flags,
               '-I'+str(source/'firmware/src'), '-I'+str(source/'firmware/src/dsp'),
               '-I'+str(source/'experiments/original_synth'),
               '-I'+str(source/'experiments/wobble')]
    if sanitize:
        command += ['-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    executable = 'runner-count' if count_width else 'runner'
    if count_width:
        command += ['-DX0X_PEAK_COUNT_WIDTH', '-Wl,--wrap=original_pw_width']
    command += [str(ROOT/'tests/host/synth_peak_test.c'),
                *(str(source/u) for u in units), '-lm', '-o', str(out/executable)]
    (out/(executable+'-command.json')).write_text(json.dumps(command, indent=2)+'\n')
    subprocess.run(command, check=True)


def summary(path):
    groups = {}
    for row in csv.DictReader(path.open()):
        groups.setdefault((row['scenario'], row['notes']), []).append(row)
    result = {}
    for (scenario, notes), rows in groups.items():
        cpu = [float(r['cpu_ns']) for r in rows]
        wall = [float(r['wall_ns']) for r in rows]
        result[f'{scenario}/{notes}'] = {
            'max_block_cpu_ns': max(cpu), 'max_block_wall_ns': max(wall),
            'max_pair_cpu_ns': max(sum(cpu[i:i+2]) for i in range(0, len(cpu), 2)),
            'avg_ns_per_sample': statistics.mean(cpu)/256,
            'worst_cpu_block': rows[cpu.index(max(cpu))]['block'],
            'cold_adoption_cpu_ns': max(float(r['cold_init_cpu_ns']) for r in rows),
        }
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT)
    parser.add_argument('--reference-source', type=Path)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--rounds', type=int, default=7)
    parser.add_argument('--exact', action='store_true', help='exact software FMA, without target fast-FMA flag')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    if args.rounds < 1:
        parser.error('--rounds must be positive')
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    if hasattr(os, 'sched_getaffinity'):
        os.sched_setaffinity(0, {min(os.sched_getaffinity(0))})
    sides = {'candidate': args.source.resolve()}
    if args.reference_source:
        sides['reference'] = args.reference_source.resolve()
    results = {side: [] for side in sides}
    for side, source in sides.items():
        folder = out/side
        folder.mkdir()
        build(source, folder, args.exact, args.sanitize)
    # Keep call-count instrumentation out of both timed executables.
    folder = out/'candidate'
    build(sides['candidate'], folder, args.exact, args.sanitize, count_width=True)
    with (folder/'count.log').open('w') as log:
        subprocess.run([str(folder/'runner-count'), str(folder/'count.csv'),
                        str(folder/'count.f32'), '--check-precompute'], check=True, stdout=log)
    for round_no in range(args.rounds):
        for side in (list(sides) if round_no % 2 == 0 else list(sides)[::-1]):
            folder = out/side
            csv_path = folder/f'round-{round_no}.csv'
            pcm_path = folder/f'round-{round_no}.f32'
            command = [str(folder/'runner'), str(csv_path), str(pcm_path)]
            with (folder/f'round-{round_no}.log').open('w') as log:
                subprocess.run(command, check=True, stdout=log)
            results[side].append(summary(csv_path))
        if 'reference' in sides:
            reference = (out/'reference'/f'round-{round_no}.f32').read_bytes()
            candidate = (out/'candidate'/f'round-{round_no}.f32').read_bytes()
            if reference != candidate:
                raise SystemExit(f'FAIL: PCM differs in round {round_no}')
    report = {'host_only': True, 'sample_rate': 44100, 'render_frames': 256,
              'dma_frames': 512, 'dma_deadline_ns': 512*1e9/44100,
              'rounds': args.rounds, 'exact_fma': args.exact,
              'sanitized': args.sanitize, 'pcm_equal': 'reference' in sides,
              'pcm_bytes_per_round': (out/'candidate/round-0.f32').stat().st_size,
              'pcm_sha256': hashlib.sha256((out/'candidate/round-0.f32').read_bytes()).hexdigest(),
              'timing': results,
              'limits': 'Synth closure only. No AC79 timing, ISR contention, drums, mixer, UI, flash, USB, or DMA execution.'}
    (out/'result.json').write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: no pulse-width recomputation during note/control work')
    if 'reference' in sides:
        print('PASS: byte-identical reference/candidate PCM')
    print(out/'result.json')


if __name__ == '__main__':
    main()
