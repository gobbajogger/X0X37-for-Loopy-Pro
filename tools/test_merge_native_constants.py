#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import hashlib
from pathlib import Path
import re
import struct
import tempfile
import unittest
import merge_native_constants as m

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / 'firmware/src/dsp/ima_adpcm.h'
PAYLOAD = bytes(map(int, re.search(r'IMA_ROUND_CORR\[64\]\s*=\s*\{(.*?)\}', HEADER.read_text(), re.S)[1].replace('\n', '').strip().strip(',').split(',')))

def obj(payload=PAYLOAD, flags=2, name='.rodata.IMA_ROUND_CORR', reloc=False):
    names = b'\0.shstrtab\0' + name.encode() + b'\0.rela.table\0'
    table_start = (52 + len(names) + 3) & ~3
    directory = (table_start + len(payload) + 3) & ~3
    count = 4 if reloc else 3
    d = bytearray(directory + 40 * count)
    d[:16] = b'\x7fELF\x01\x01\x01' + b'\0' * 9
    struct.pack_into('<HHIIIIIHHHHHH', d, 16, 1, 241, 1, 0, 0, directory, 0, 52, 0, 0, 40, count, 1)
    d[52:52 + len(names)] = names
    d[table_start:table_start + len(payload)] = payload
    struct.pack_into('<10I', d, directory + 40, 1, 3, 0, 0, 52, len(names), 0, 0, 1, 0)
    struct.pack_into('<10I', d, directory + 80, 11, 1, flags, 0, table_start, len(payload), 0, 0, 4, 0)
    if reloc:
        struct.pack_into('<10I', d, directory + 120, 11 + len(name) + 1, 4, 0, 0, table_start, 0, 0, 2, 4, 12)
    return d

class TestMerge(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
    def put(self, name, data):
        p = self.root / name; p.write_bytes(data); return p
    def test_pinned_input(self):
        self.assertEqual((len(PAYLOAD), hashlib.sha256(PAYLOAD).hexdigest()), m.TABLES['IMA_ROUND_CORR'])
    def test_duplicate_metadata_only_and_idempotent(self):
        before = obj(); a = self.put('a.o', before); b = self.put('b.o', before)
        self.assertEqual(len(m.merge_constants([a, b])), 2)
        after = a.read_bytes(); sh = struct.unpack_from('<I', after, 32)[0] + 80
        permitted = set(range(sh + 8, sh + 12)) | set(range(sh + 36, sh + 40))
        self.assertTrue(all(i in permitted for i, (x, y) in enumerate(zip(before, after)) if x != y))
        self.assertEqual(struct.unpack_from('<I', after, sh + 8)[0], 18)
        self.assertEqual(struct.unpack_from('<I', after, sh + 36)[0], 64)
        self.assertEqual(m._tables(a)[1][0][2], PAYLOAD)
        m.merge_constants([a, b]); self.assertEqual(a.read_bytes(), after)
    def test_unique_unchanged(self):
        d = obj(); p = self.put('a.o', d); self.assertEqual(m.merge_constants([p]), []); self.assertEqual(p.read_bytes(), d)
    def test_unlisted_unchanged(self):
        d = obj(name='.rodata.other'); p = self.put('a.o', d); self.assertEqual(m.merge_constants([p]), []); self.assertEqual(p.read_bytes(), d)
    def test_reject_invalid_and_validate_all_before_write(self):
        cases = [obj(flags=3), obj(payload=PAYLOAD[:-1]), obj(payload=b'\0'*64), obj(reloc=True)]
        bad_machine = obj(); struct.pack_into('<H', bad_machine, 18, 3); cases.append(bad_machine)
        bad_dir = obj(); struct.pack_into('<I', bad_dir, 32, 0xffffffff); cases.append(bad_dir)
        cases.append(obj()[:-1])
        bad_endian = obj(); bad_endian[5] = 2; cases.append(bad_endian)
        for off, fmt, value in [(6, 'B', 0), (20, '<I', 0), (40, '<H', 51), (28, '<I', 52), (44, '<H', 1)]:
            bad = obj(); struct.pack_into(fmt, bad, off, value); cases.append(bad)
        for relative, value in [(44, 1), (112, 3), (112, 0), (96, 0), (96, 52), (92, 1)]:
            bad = obj(); directory = struct.unpack_from('<I', bad, 32)[0]
            struct.pack_into('<I', bad, directory + relative, value); cases.append(bad)
        bad = obj(payload=bytes(52), name='.rodata.fx_beats')
        directory = struct.unpack_from('<I', bad, 32)[0]
        struct.pack_into('<I', bad, directory + 112, 16); cases.append(bad)
        with self.assertRaisesRegex(ValueError, 'bounds/alignment'):
            m._tables(self.put('bad-entry-alignment.o', bad))
        for n, data in enumerate(cases):
            with self.subTest(n=n):
                a = self.put('a.o', obj()); b = self.put('b.o', obj()); bad = self.put('bad.o', data)
                with self.assertRaises(ValueError): m.merge_constants([a, b, bad])
                self.assertEqual(a.read_bytes(), obj()); self.assertEqual(b.read_bytes(), obj())
if __name__ == '__main__': unittest.main()
