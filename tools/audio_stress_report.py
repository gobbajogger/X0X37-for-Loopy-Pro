#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Summarize observed host half-block peaks; never certify target headroom."""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import statistics

FRAMES = 512
SAMPLE_RATE = 44100
DEADLINE_NS = FRAMES * 1e9 / SAMPLE_RATE


def file_sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(65536), b''):
            digest.update(chunk)
    return digest.hexdigest()


def summarize(rows):
    if not rows:
        raise ValueError('no measured blocks')
    columns = ('cpu_ns', 'wall_ns', 'chicago_cpu_ns', 'miami_cpu_ns', 'ui_cpu_ns')
    values = {name: [] for name in columns}
    for index, row in enumerate(rows):
        if int(row['block']) != index:
            raise ValueError('blocks must be contiguous, unique and start at zero')
        for name in columns:
            value = float(row[name])
            if not math.isfinite(value) or value < 0:
                raise ValueError(f'invalid {name} in block {index}')
            values[name].append(value)

    def metric(name, fraction):
        samples = values[name]
        peak = max(samples)
        return {
            'peak_ns': peak,
            'worst_block': samples.index(peak),
            'average_ns': statistics.mean(samples),
            'peak_deadline_percent': peak / DEADLINE_NS * 100,
            'margin_ns': DEADLINE_NS - peak,
            'limit_fraction': fraction,
            'over_limit_blocks': sum(v > DEADLINE_NS * fraction for v in samples),
            'over_deadline_blocks': sum(v > DEADLINE_NS for v in samples),
        }

    result = {
        'measurement': 'host; instrumented application, not a hardware ISR',
        'frames_per_half': FRAMES, 'sample_rate': SAMPLE_RATE,
        'deadline_ns': DEADLINE_NS, 'blocks': len(rows),
        'render_cpu': metric('cpu_ns', .60),
        'render_wall': metric('wall_ns', .60),
        'chicago_cpu': metric('chicago_cpu_ns', .15),
        'miami_cpu': metric('miami_cpu_ns', .15),
        'ui_cpu': metric('ui_cpu_ns', 1),
        'hardware_dropouts': None, 'target_acceptance': 'NOT_RUN',
        'limits': [
            'CPU0 and CPU1 work is serialized by the host platform substitute.',
            'USB/BLE messages use production consumers but no radio/USB driver or interrupts.',
            'UI runs between audio halves; IRQ nesting, SPI/XIP contention and DMA are absent.',
            'UI CPU is reported separately and is outside the timed audio half.',
            'Observed host deadline overruns are not measured hardware dropouts.',
            'Observed maxima are workload samples, not a worst-case execution-time proof.',
        ],
    }
    result['host_timing_within_requested_limits'] = all(
        result[name]['over_limit_blocks'] == 0
        for name in ('render_cpu', 'render_wall', 'chicago_cpu', 'miami_cpu'))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--csv', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--pcm', type=Path)
    parser.add_argument('--require-host-limits', action='store_true',
                        help='exit nonzero if an observed host peak exceeds the requested limits')
    args = parser.parse_args()
    with args.csv.open(newline='') as stream:
        result = summarize(list(csv.DictReader(stream)))
    result['csv_sha256'] = file_sha256(args.csv)
    if args.pcm:
        expected = result['blocks'] * FRAMES * 2 * 4
        if args.pcm.stat().st_size != expected:
            raise ValueError(f'PCM length must be {expected} bytes')
        result['pcm_bytes'] = expected
        result['pcm_sha256'] = file_sha256(args.pcm)
    args.out.write_text(json.dumps(result, indent=2) + '\n')
    peak = result['render_cpu']['peak_ns'] / 1e6
    margin = result['render_cpu']['margin_ns'] / 1e6
    status = 'PASS' if result['host_timing_within_requested_limits'] else 'FAIL'
    print(f'Host audio half peak {peak:.6f} ms; margin {margin:.6f} ms; host timing limits {status}; target NOT_RUN')
    return int(args.require_host_limits and not result['host_timing_within_requested_limits'])


if __name__ == '__main__':
    raise SystemExit(main())
