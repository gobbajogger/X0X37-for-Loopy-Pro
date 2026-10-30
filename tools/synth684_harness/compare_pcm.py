#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Fail-closed, bit-exact original-vs-candidate stereo binary32 PCM comparison.

Accepts raw little-endian stereo .f32 and IEEE-float stereo .wav captures;
compares matched basenames without resampling, alignment, normalization or
amplitude adjustment. Peak/RMS/gain fit are diagnostics, never acceptance.
The nonzero exit code means a missing file, incompatible format, or a single
mismatched sample bit. No external Python packages required.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys


class PCMError(ValueError):
    pass


def payload(path, rate):
    data = path.read_bytes()
    if path.suffix.lower() == '.f32':
        if len(data) % 8:
            raise PCMError(f'{path}: stereo .f32 payload not divisible by 8')
        return data, rate
    if path.suffix.lower() != '.wav':
        raise PCMError(f'{path}: expected stereo .f32 or floating-point .wav')
    if len(data) < 12 or data[:4] != b'RIFF' or data[8:12] != b'WAVE':
        raise PCMError(f'{path}: not a RIFF/WAVE file')
    if int.from_bytes(data[4:8], 'little') + 8 > len(data):
        raise PCMError(f'{path}: truncated RIFF')
    start, fmt, chunk = 12, None, None
    while start + 8 <= len(data):
        name, n = struct.unpack_from('<4sI', data, start)
        begin, end = start + 8, start + 8 + n
        if end > len(data):
            raise PCMError(f'{path}: truncated {name!r} chunk')
        if name == b'fmt ':
            if fmt is not None or n < 16:
                raise PCMError(f'{path}: invalid/multiple fmt chunks')
            tag, channels, sample_rate, byte_rate, block_align, bits = struct.unpack_from('<HHIIHH', data, begin)
            if tag == 0xfffe and n >= 40:
                # WaveFormatExtensible: IEEE FLOAT subformat GUID.
                guid = data[begin + 24:begin + 40]
                if guid == bytes.fromhex('0300000000001000800000aa00389b71'):
                    tag = 3
            fmt = (tag, channels, sample_rate, byte_rate, block_align, bits)
        if name == b'data':
            if chunk is not None:
                raise PCMError(f'{path}: multiple data chunks are unsupported')
            chunk = data[begin:end]
        start = end + (n & 1)
    if fmt is None or chunk is None:
        raise PCMError(f'{path}: missing audio format/data')
    tag, channels, sample_rate, byte_rate, block_align, bits = fmt
    if (tag, channels, block_align, bits) != (3, 2, 8, 32):
        raise PCMError(f'{path}: requires stereo IEEE-float 32-bit WAV, got {fmt}')
    if sample_rate < 1 or byte_rate != 8 * sample_rate or len(chunk) % 8:
        raise PCMError(f'{path}: invalid WAV frame/rate metadata')
    return chunk, sample_rate


def _metrics(data):
    stats = [{'peak': 0., 'sum2': 0., 'n': 0} for _ in range(2)]
    for i, (v,) in enumerate(struct.iter_unpack('<f', data)):
        if not math.isfinite(v):
            raise PCMError(f'non-finite sample at frame {i // 2}, channel {i % 2}')
        s = stats[i % 2]
        s['peak'] = max(s['peak'], abs(v))
        s['sum2'] += float(v) * float(v)
        s['n'] += 1
    return [{'peak': s['peak'], 'rms': math.sqrt(s['sum2'] / s['n']) if s['n'] else 0.}
            for s in stats]


def compare(reference, candidate, rate):
    a, ar = payload(reference, rate)
    b, br = payload(candidate, rate)
    if ar != br:
        raise PCMError(f'{reference.name}: sample-rate mismatch {ar} vs {br}, no resampling permitted')
    if len(a) != len(b):
        raise PCMError(f'{reference.name}: frame-count mismatch {len(a) // 8} vs {len(b) // 8}')
    if not a:
        raise PCMError(f'{reference.name}: empty PCM is not evidence')
    am = _metrics(a)
    bm = _metrics(b)
    # One pass over exact binary32 words; -0 differs from +0 even if sample
    # magnitudes are identical. Gain-fit ratios are approximate diagnostics.
    mismatch, first, maximum = 0, None, 0.
    sums = [{'cross': 0., 'candidate2': 0., 'residual2': 0., 'reference2': 0.}
            for _ in range(2)]
    for i in range(0, len(a), 4):
        ac, bc = a[i:i+4], b[i:i+4]
        x, y = struct.unpack('<f', ac)[0], struct.unpack('<f', bc)[0]
        ch = i // 4 % 2
        s = sums[ch]
        s['cross'] += float(x) * float(y)
        s['candidate2'] += float(y) * float(y)
        s['reference2'] += float(x) * float(x)
        maximum = max(maximum, abs(float(x) - float(y)))
        if ac != bc:
            mismatch += 1
            if first is None:
                first = {'frame': i // 8, 'channel': 'L' if ch == 0 else 'R',
                         'reference_bits': f'0x{int.from_bytes(ac, "little"):08x}',
                         'candidate_bits': f'0x{int.from_bytes(bc, "little"):08x}',
                         'reference_value': x, 'candidate_value': y}
    channel = []
    for i, s in enumerate(sums):
        gain = s['cross'] / s['candidate2'] if s['candidate2'] else None
        residue = (s['reference2'] - 2 * gain * s['cross'] + gain * gain * s['candidate2']) if gain is not None else None
        n = len(a) // 8
        channel.append({
            'reference_peak': am[i]['peak'], 'candidate_peak': bm[i]['peak'],
            'reference_rms': am[i]['rms'], 'candidate_rms': bm[i]['rms'],
            'peak_reference_over_candidate': am[i]['peak'] / bm[i]['peak'] if bm[i]['peak'] else None,
            'rms_reference_over_candidate': am[i]['rms'] / bm[i]['rms'] if bm[i]['rms'] else None,
            'gain_fit_reference_over_candidate': gain,
            'relative_residual_after_gain_fit': (math.sqrt(max(0., residue / n)) / am[i]['rms'])
            if residue is not None and am[i]['rms'] else None,
        })
    return {'name': reference.stem, 'sample_rate': ar, 'frames': len(a) // 8,
            'reference_pcm_sha256': hashlib.sha256(a).hexdigest(),
            'candidate_pcm_sha256': hashlib.sha256(b).hexdigest(),
            'bit_exact': mismatch == 0, 'mismatched_sample_words': mismatch,
            'first_mismatch': first, 'max_absolute_sample_difference': maximum,
            'channels': {'L': channel[0], 'R': channel[1]}}


def _find_files(root):
    if root.is_symlink():
        raise PCMError(f'{root}: symlink inputs rejected')
    if root.is_file():
        if root.suffix.lower() not in ('.f32', '.wav'):
            raise PCMError(f'{root}: expected .f32/.wav')
        return {root.stem: root}
    if not root.is_dir():
        raise PCMError(f'{root}: missing input directory')
    files = {}
    for path in sorted(root.iterdir()):
        if path.is_symlink():
            raise PCMError(f'{path}: symlink inputs rejected')
        if path.is_file() and path.suffix.lower() in ('.f32', '.wav'):
            if path.stem in files:
                raise PCMError(f'{root}: duplicate WAV/.f32 basename {path.stem}')
            files[path.stem] = path
    if not files:
        raise PCMError(f'{root}: no .f32/.wav captures')
    return files


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('reference', type=Path, help='original Miami golden directory/file')
    ap.add_argument('candidate', type=Path, help='candidate rendered PCM directory/file')
    ap.add_argument('--rate', type=int, default=44100, help='raw .f32 sample rate (default 44100)')
    ap.add_argument('--require-count', type=int, help='fail if not exactly N matched scenarios')
    ap.add_argument('--json', type=Path, help='write structured metrics, including failures')
    args = ap.parse_args(argv)
    try:
        if args.rate < 1 or args.require_count is not None and args.require_count < 1:
            raise PCMError('rate/count must be positive')
        reference, candidate = _find_files(args.reference), _find_files(args.candidate)
        if reference.keys() != candidate.keys():
            raise PCMError(f'file sets differ; golden-only={sorted(reference.keys() - candidate.keys())}; '
                           f'candidate-only={sorted(candidate.keys() - reference.keys())}')
        if args.require_count is not None and len(reference) != args.require_count:
            raise PCMError(f'expected {args.require_count} files, found {len(reference)}')
        files = [compare(reference[k], candidate[k], args.rate) for k in sorted(reference)]
        exact = all(f['bit_exact'] for f in files)
        report = {'status': 'PASS' if exact else 'FAIL', 'bit_exact': exact,
                  'scenarios': len(files), 'files': files,
                  'note': 'Peak/RMS and fitted gain are diagnostic only; never waive bit equality.'}
        rc = 0 if exact else 1
    except (PCMError, OSError, struct.error) as exc:
        report, rc = {'status': 'ERROR', 'bit_exact': False, 'error': str(exc)}, 2
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, indent=2, allow_nan=False) + '\n')
    print(json.dumps(report, indent=2, allow_nan=False))
    return rc


if __name__ == '__main__':
    sys.exit(main())
