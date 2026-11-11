#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Source-executed WAV import integrity controls: temporary real RIFF files, no MIDI.

Default: tools/sampleio.py in the repository checkout. For isolated source-window
preflight pass an explicit source filename; that is NOT complete-root qualification.
"""
import importlib.util
import math
import pathlib
import struct
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SOURCE = pathlib.Path(sys.argv.pop(1)) if len(sys.argv) > 1 else ROOT / 'tools/sampleio.py'
spec = importlib.util.spec_from_file_location('wav_import_under_test', SOURCE)
assert spec and spec.loader
sio = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sio)


def riff(chunks):
    body = b'WAVE' + b''.join(k + struct.pack('<I', len(v)) + v + (b'\0' if len(v) & 1 else b'') for k, v in chunks)
    return b'RIFF' + struct.pack('<I', len(body)) + body


def fmt(tag=1, bits=16, channels=1, rate=22050, align=None):
    bps = bits // 8
    if align is None:
        align = channels * bps
    return struct.pack('<HHIIHH', tag, channels, rate, rate * align, align, bits)


def extensible(tag=1, bits=24, channels=2, valid=None, cbsize=22, guid=None):
    if valid is None:
        valid = bits
    if guid is None:
        guid = struct.pack('<I', tag) + bytes.fromhex('00001000800000aa00389b71')
    return fmt(0xfffe, bits, channels) + struct.pack('<HHI', cbsize, valid, 0) + guid


class WavImportSafety(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='wav-integrity-')
        self.addCleanup(self.temp.cleanup)
        self.file = pathlib.Path(self.temp.name) / 'sample.wav'

    def load(self, format_bytes, samples=b'\x00\x00', extra=()):
        self.file.write_bytes(riff([(b'fmt ', format_bytes), *extra, (b'data', samples)]))
        return sio.read_any_wav(self.file)

    def test_pcm_positive_formats(self):
        cases = [(8, b'\x80\xff', [0.0, 127/128]),
                 (16, struct.pack('<2h', 0, 16384), [0.0, 0.5]),
                 (24, b'\x00\x00\x00\x00\x00\x40', [0.0, 0.5]),
                 (32, struct.pack('<2i', 0, 1073741824), [0.0, 0.5])]
        for bits, data, expected in cases:
            with self.subTest(bits=bits):
                rate, values = self.load(fmt(bits=bits), data)
                self.assertEqual(rate, 22050)
                self.assertEqual(values, expected)

    def test_float_positive_formats(self):
        for bits, code in ((32, 'f'), (64, 'd')):
            with self.subTest(bits=bits):
                rate, values = self.load(fmt(tag=3, bits=bits), struct.pack('<2'+code, 0.25, -0.75))
                self.assertEqual(rate, 22050)
                self.assertEqual(values, [0.25, -0.75])

    def test_extensible_standard_guids_allowed(self):
        self.assertEqual(self.load(extensible(1,16,1), struct.pack('<h',8192))[1], [0.25])
        self.assertEqual(self.load(extensible(3,32,1), struct.pack('<f',0.25))[1], [0.25])

    def test_bad_format_tags_and_depths_rejected(self):
        cases = [fmt(bits=12), fmt(bits=7), fmt(bits=40), fmt(tag=3,bits=16), fmt(tag=3,bits=24), fmt(tag=6,bits=16)]
        for index, f in enumerate(cases):
            with self.subTest(case=index):
                with self.assertRaises(ValueError): self.load(f,b'\0'*12)

    def test_zero_channels_and_rate_rejected(self):
        for f in (fmt(channels=0),fmt(rate=0)):
            with self.assertRaises(ValueError): self.load(f,b'\0'*12)

    def test_bad_or_truncated_fmt_rejected_cleanly(self):
        for f in (b'\x01', b'\0'*15, b'\0'*17):
            with self.assertRaises(ValueError): self.load(f)
        self.file.write_bytes(b'RIFF' + b'\0'*4 + b'WAVE' + b'fmt ' + struct.pack('<I',100) + fmt())
        with self.assertRaises(ValueError): sio.read_any_wav(self.file)

    def test_frame_alignment_rejected(self):
        for align in (1, 3, 5):
            with self.subTest(align=align):
                with self.assertRaises(ValueError): self.load(fmt(bits=16,channels=2,align=align), b'\0'*16)

    def test_extensible_extension_contract_rejected(self):
        cases = [extensible()[:30], extensible(cbsize=8), extensible(valid=0), extensible(valid=32),
                 extensible(guid=bytes.fromhex('0100000000001000800000aa00389b70')),
                 extensible(guid=bytes.fromhex('0100010000001000800000aa00389b71'))]
        for index, f in enumerate(cases):
            with self.subTest(case=index):
                with self.assertRaises(ValueError): self.load(f,b'\0'*16)

    def test_float_payload_nonfinite_rejected(self):
        for bits, code in ((32, 'f'), (64, 'd')):
            for val in (math.nan,math.inf,-math.inf):
                with self.subTest(bits=bits, val=val):
                    with self.assertRaisesRegex(ValueError,'NaN|infinity'):
                        self.load(fmt(tag=3,bits=bits), struct.pack('<'+code,val))

    def test_float64_stereo_mix_overflow_rejected(self):
        with self.assertRaisesRegex(ValueError,'mix overflow'):
            self.load(fmt(tag=3,bits=64,channels=2),struct.pack('<2d',1e308,1e308))

    def test_float64_multichannel_mix_overflow_rejected(self):
        with self.assertRaisesRegex(ValueError,'mix overflow'):
            self.load(fmt(tag=3,bits=64,channels=3),struct.pack('<3d',1e308,1e308,-1e308))

    def test_finite_mix_preserves_order_and_values(self):
        self.assertEqual(self.load(fmt(tag=3,bits=64,channels=2),struct.pack('<2d',0.75,-0.25))[1],[0.25])
        self.assertEqual(self.load(fmt(tag=3,bits=64,channels=3),struct.pack('<3d',0.75,-0.25,0.25))[1],[0.25])

    def test_resampling_overflow_rejected(self):
        with self.assertRaisesRegex(ValueError,'resampling overflow'):
            sio.resample([1e308]*8,44100,22050)

    def test_normal_resampling_unmodified(self):
        self.assertEqual(sio.resample([0.0,0.5,1.0,0.5],22050,22050),[0.0,0.5,1.0,0.5])
        self.assertTrue(all(math.isfinite(v) for v in sio.resample([0.0,0.5,1.0,0.5],44100,22050)))

    def test_truncated_data_allowed(self):
        self.file.write_bytes(riff([(b'fmt ',fmt()),(b'data',struct.pack('<2h',1,2))])[:-1])
        rate,values=sio.read_any_wav(self.file)
        self.assertEqual(rate,22050)
        self.assertEqual(values,[1/32768])


if __name__ == '__main__':
    unittest.main(verbosity=2)