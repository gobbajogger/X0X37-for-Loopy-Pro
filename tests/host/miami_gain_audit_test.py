#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only revision-13 Miami mixer-gain forensic regression."""
import importlib.util
import json
from pathlib import Path
import struct
import sys
import unittest
import zlib

TOOL = Path(__file__).resolve().parents[2] / 'tools/miami_gain_audit.py'
spec = importlib.util.spec_from_file_location('miami_gain_audit', TOOL)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def make_project(level=100, loud=1, output=101, mask=0, rate=0):
    data = bytearray(mod.SOUND_START + mod.SOUND_SIZE + 200)
    struct.pack_into('<I', data, 32, 0x50305830)
    struct.pack_into('<I', data, 36, 1)
    data[50] = 2  # stereo, in settings
    data[51] = 13 # sound revision
    data[57] = loud
    data[58] = 1  # USB FULL
    data[54] = rate  # MIDI channel for part 303A, 0 means default 2
    for v, kind in ((0, 1), (1, 0)):
        at = mod.SOUND_START + (2 * mod.VOICES + v) * mod.PARAMS
        data[at+26:at+28] = b'OS'
        data[at+28] = 2
        data[at+29] = kind
        data[at+30] = 0
        data[at+14] = output
        if v == 0:
            data[at+22:at+26] = mask.to_bytes(4, 'little')
        mix = mod.SOUND_START + (mod.MIX*mod.VOICES + 2+v)*mod.PARAMS
        data[mix] = level if v == 0 else 100
        data[mix + 3] = 64
    data[:4] = b'X0XP'
    struct.pack_into('<I', data, 4, 1)
    struct.pack_into('<I', data, 8, len(data)-32)
    struct.pack_into('<I', data, 16, 1)
    struct.pack_into('<I', data, 20, 3)
    struct.pack_into('<I', data, 24, 13)
    struct.pack_into('<I', data, 12, zlib.crc32(data[32:]))
    return bytes(data)


def make_portable(raw):
    meta = {'format': 'x0xproject', 'version':1,
            'project':{'offset':0, 'length':len(raw), 'crc32':zlib.crc32(raw)},
            'archive':None}
    j=json.dumps(meta).encode()
    out=bytearray(b'X0XPROJ\0' + struct.pack('<HHI',1,0,len(j)) + j + raw)
    out += struct.pack('<I', zlib.crc32(out))
    return bytes(out)


class GainAuditTest(unittest.TestCase):
    def test_defaults_unity_loud_applies_to_both(self):
        result=mod.inspect(mod.unpack(make_project()))
        self.assertEqual('LOUD',result['output_setting'])
        self.assertEqual('FULL',result['usb_audio_setting'])
        self.assertEqual(2,result['midi_part_channel_2'])
        self.assertEqual(1,result['parts'][0]['mixer_gain_linear'])
        self.assertEqual(1,result['parts'][1]['mixer_gain_linear'])
        self.assertFalse(result['parts'][0]['miami_output_explicitly_edited'])

    def test_level20_explains_roughly_28db(self):
        result=mod.inspect(make_project(level=20))
        self.assertAlmostEqual(-27.9588001734, result['parts'][0]['mixer_gain_db'],places=6)
        self.assertEqual(1, result['parts'][1]['mixer_gain_linear'])

    def test_quiet_velocity_also_causes_same_level(self):
        import math
        loss=20*math.log10((20/100)**2)
        self.assertAlmostEqual(-27.9588001734,loss,places=6)

    def test_macro_edit_is_reported_not_applied(self):
        result=mod.inspect(make_project(output=20,mask=1<<14))
        self.assertTrue(result['parts'][0]['miami_output_explicitly_edited'])
        self.assertEqual(20,result['parts'][0]['display_output_byte_0_127'])

    def test_portable_read_only(self):
        self.assertEqual(mod.inspect(make_project()),mod.inspect(mod.unpack(make_portable(make_project()))))

    def test_reject_bad_crc(self):
        data=bytearray(make_project());data[mod.SOUND_START+100]^=1
        with self.assertRaises(mod.InvalidProject):mod.unpack(bytes(data))

    def test_reject_unknown_revision(self):
        data=bytearray(make_project());struct.pack_into('<I',data,24,11)
        with self.assertRaises(mod.InvalidProject):mod.unpack(bytes(data))

    def test_reject_portable_outer_crc(self):
        data=bytearray(make_portable(make_project()));data[-1]^=1
        with self.assertRaises(mod.InvalidProject):mod.unpack(bytes(data))

    def test_reject_portable_inner_crc(self):
        raw=bytearray(make_project());raw[-1]^=1
        data=make_portable(bytes(raw))
        with self.assertRaises(mod.InvalidProject):mod.unpack(data)

    def test_reject_wrong_engine_identity(self):
        data=bytearray(make_project());at=mod.SOUND_START+(2*mod.VOICES)*mod.PARAMS
        data[at+29]=0;struct.pack_into('<I',data,12,zlib.crc32(data[32:]))
        with self.assertRaises(mod.InvalidProject):mod.inspect(mod.unpack(bytes(data)))

    def test_reject_invalid_stored_level(self):
        data=bytearray(make_project());at=mod.SOUND_START+(mod.MIX*mod.VOICES+2)*mod.PARAMS
        data[at]=255;struct.pack_into('<I',data,12,zlib.crc32(data[32:]))
        with self.assertRaises(mod.InvalidProject):mod.inspect(mod.unpack(bytes(data)))


if __name__=='__main__':unittest.main()
