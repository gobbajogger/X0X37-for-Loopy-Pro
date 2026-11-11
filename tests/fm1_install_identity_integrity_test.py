#!/usr/bin/env python3
"""Offline OTA identity envelope checks; no MIDI backend, device or captured bytes."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import fm1_install as I


def raw(text='SMK-37 Elite_900'):
    payload = text.encode().ljust(27, b'\0')
    return bytearray([0, 0x59, 0x11, 27, 0, 0]) + payload + bytes([~sum(payload) & 255])


def frame(body):
    return b'\xf0' + I.pack7(body) + b'\xf7'


class IdentityIntegrity(unittest.TestCase):
    def test_valid_contract_frames(self):
        for text in ('FM-1_015', 'FM-1_016', 'SMK-37 Pro_016', 'SMK-37 Elite_016',
                     'SMK-37 Elite_900', 'ota-SMK-37 Elite_900', 'Felucca_000', 'SMK-37 Elite_901'):
            with self.subTest(text=text):
                self.assertEqual(I.parse_identity(frame(raw(text))).text, text)

    def test_corruption(self):
        mutations = []
        bad = raw(); bad[-1] ^= 1; mutations.append(('checksum', frame(bad)))
        bad = raw(); bad[20] = ord('1'); mutations.append(('version unchanged checksum', frame(bad)))
        for n in (0, 26, 28, 283, 65563):
            bad = raw(); bad[3:6] = n.to_bytes(3, 'little'); mutations.append((f'length {n}', frame(bad)))
        good = frame(raw())
        mutations += [('missing start', good[1:]), ('missing end', good[:-1]), ('truncated', good[:-2]+b'\xf7'), ('extra', good[:-1]+b'\0\xf7')]
        bad = bytearray(good); bad[2] |= 128; mutations.append(('status', bad))
        bad = bytearray(good); bad[-2] |= 64; mutations.append(('padding', bad))
        for label, pkt in mutations:
            with self.subTest(label=label):
                self.assertIsNone(I.parse_identity(pkt))


if __name__ == '__main__':
    unittest.main(verbosity=2)
