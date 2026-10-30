#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Standalone deterministic tests for Miami golden peak/RMS/bit comparison."""
import importlib.util
import json
import math
from pathlib import Path
import struct
import tempfile
import unittest
from unittest import mock

SOURCE = Path(__file__).resolve().parents[2] / 'tools/synth684_harness/compare_pcm.py'
sp = importlib.util.spec_from_file_location('miami_pcm_compare', SOURCE)
pcm = importlib.util.module_from_spec(sp)
sp.loader.exec_module(pcm)


def data(frames):
    return struct.pack('<' + 'f' * len(frames), *frames)


def wav_f32(frames, rate=44100):
    p = data(frames)
    fmt = struct.pack('<HHIIHH', 3, 2, rate, 8 * rate, 8, 32)
    return b'RIFF' + struct.pack('<I', 4 + (8 + len(fmt)) + (8 + len(p))) + b'WAVE' + b'fmt ' + struct.pack('<I', len(fmt)) + fmt + b'data' + struct.pack('<I', len(p)) + p


class PCMCompareTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / 'golden').mkdir()
        (self.root / 'candidate').mkdir()

    def pair(self, name, original, candidate):
        a = self.root / 'golden' / f'{name}.f32'
        b = self.root / 'candidate' / f'{name}.f32'
        a.write_bytes(original)
        b.write_bytes(candidate)
        return a, b

    def test_exact_waveform(self):
        values = [0.5, -0.25, 0.75, -0.125, 0.0, 0.0]
        a, b = self.pair('exact', data(values), data(values))
        s = pcm.compare(a, b, 44100)
        self.assertTrue(s['bit_exact'])
        self.assertEqual(0, s['mismatched_sample_words'])
        self.assertEqual(0.75, s['channels']['L']['reference_peak'])

    def test_quiet_candidate_is_not_accepted_as_gain_fit(self):
        a, b = self.pair('quiet', data([0.5, -0.5, 0.25, -0.25]), data([0.25, -0.25, 0.125, -0.125]))
        s = pcm.compare(a, b, 44100)
        self.assertFalse(s['bit_exact'])
        self.assertEqual(4, s['mismatched_sample_words'])
        self.assertEqual(2.0, s['channels']['L']['gain_fit_reference_over_candidate'])
        self.assertEqual(0., s['channels']['L']['relative_residual_after_gain_fit'])

    def test_signed_zero_fails_despite_identical_rms(self):
        a, b = self.pair('zero', struct.pack('<II', 0, 0), struct.pack('<II', 0x80000000, 0))
        s = pcm.compare(a, b, 44100)
        self.assertFalse(s['bit_exact'])
        self.assertEqual(0., s['channels']['L']['reference_rms'])
        self.assertEqual(0, s['first_mismatch']['frame'])

    def test_tonal_or_phase_difference_does_not_look_like_gain(self):
        a, b = self.pair('tone', data([0.5, 0.5, -0.25, -0.25]), data([0.5, 0.5, 0.25, 0.25]))
        s = pcm.compare(a, b, 44100)
        self.assertFalse(s['bit_exact'])
        self.assertGreater(s['channels']['L']['relative_residual_after_gain_fit'], 0.5)

    def test_wav_ieee_float_vs_f32(self):
        original = self.root / 'golden' / 'take.wav'
        candidate = self.root / 'candidate' / 'take.f32'
        original.write_bytes(wav_f32([0.5, -0.5, 0.0, 0.0]))
        candidate.write_bytes(data([0.5, -0.5, 0.0, 0.0]))
        self.assertTrue(pcm.compare(original, candidate, 44100)['bit_exact'])
        original.write_bytes(wav_f32([0.5, -0.5], 48000))
        with self.assertRaisesRegex(pcm.PCMError, 'sample-rate mismatch'):
            pcm.compare(original, candidate, 44100)

    def test_header_and_length_refusals(self):
        a, b = self.pair('truncated', b'\x00\x00\x00\x00', data([0., 0.]))
        with self.assertRaisesRegex(pcm.PCMError, 'not divisible'):
            pcm.compare(a, b, 44100)
        a.write_bytes(data([0., 0.]))
        b.write_bytes(data([0., 0., 0., 0.]))
        with self.assertRaisesRegex(pcm.PCMError, 'frame-count mismatch'):
            pcm.compare(a, b, 44100)

    def test_directory_count_and_missing_file_refused(self):
        self.pair('one', data([0., 0.]), data([0., 0.]))
        self.assertEqual(2, pcm.main([str(self.root / 'golden'), str(self.root / 'candidate'), '--require-count', '15']))
        (self.root / 'candidate' / 'one.f32').unlink()
        self.assertEqual(2, pcm.main([str(self.root / 'golden'), str(self.root / 'candidate')]))

    def test_fail_exit_and_report_no_false_success(self):
        self.pair('foo', data([1., 0.]), data([0.5, 0.]))
        report = self.root / 'report.json'
        with mock.patch('sys.stdout'):
            rc = pcm.main([str(self.root / 'golden'), str(self.root / 'candidate'), '--json', str(report)])
        self.assertEqual(1, rc)
        self.assertEqual('FAIL', json.loads(report.read_text())['status'])


if __name__ == '__main__':
    unittest.main()
