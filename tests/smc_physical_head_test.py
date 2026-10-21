#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Source-only physical/generator contract regressions; synthetic keys/bytes only.
No SDK, package input, raw capture, device, or target build is used.
"""
from pathlib import Path
import io
import json
import struct
import sys
import tempfile
from types import SimpleNamespace as NS
import unittest
from unittest.mock import patch
import zlib
sys.path[:0] = [str(Path(__file__).resolve().parents[1]/'tools'),
                str(Path(__file__).resolve().parents[1]/'tools/studies/smc_head_export')]
from smc_vendor_bundle_test import BundleTest
import smc_vendor_bundle as b
import fm1_install as fi
import smc_vendor_install as si
import host
from test_host import Device


def edit_entry(head, pos, edit):
    e = b._entry(head[pos:pos+32]); edit(e)
    struct.pack_into('<H', e, 0, b.pm.crc16(e[2:])); b.pm.enc(e, 0, 32)
    head[pos:pos+32] = e


def location(head, pos):
    return struct.unpack_from('<II', b._entry(head[pos:pos+32]), 4)


class Test(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        BundleTest.setUpClass()
        cls.raw, cls.head, cls.binding = BundleTest.raw, BundleTest.other_head, BundleTest.binding

    def physical(self, head):
        rows = []
        self.assertEqual(b.physical_head_key(bytes(head), diagnostics=rows.append), 0x5678)
        self.assertEqual(rows[0]['contract'], 'native-key-preserved-head-v1')
        self.assertTrue(rows[0]['key_blob_crc_valid'])
        return rows[0]

    def test_unused_payload_crc_diagnostic_bound_conversion_not_source_admission(self):
        for pos, delta in ((32, 64), (64, 34)):
            with self.subTest(pos=pos):
                h = bytearray(self.head); off, size = location(h, pos); h[off+delta] ^= 1; h = bytes(h)
                rows = self.physical(h)
                self.assertFalse(rows['payloads'][0 if pos == 32 else 1]['matches'])
                with self.assertRaisesRegex(b.BundleError, 'head data CRC'): b.head_key(h)
                diagnostics = []
                out, receipt = b.bind_head(self.raw, h, loader_binding=self.binding, diagnostics=diagnostics.append)
                self.assertEqual(b.unpack(out)[0][:0x4000], h)
                self.assertEqual(diagnostics, [rows])
                after = b.validate_bound_bundle(out, source_raw=self.raw, actual_head=h, loader_binding=self.binding)
                self.assertEqual(after['canonical_main_sha256'], b.validate_bundle(self.raw, loader_binding=self.binding)['canonical_main_sha256'])
                self.assertEqual(receipt['head_sha256'], b.sha(h))
                with self.assertRaisesRegex(b.BundleError, 'head data CRC'): b.validate_bundle(out, loader_binding=self.binding)
                with self.assertRaisesRegex(b.BundleError, 'head data CRC'): b.bind_head(out, h, loader_binding=self.binding)
                with self.assertRaises(b.BundleError): b.validate_bound_bundle(out, source_raw=self.raw, actual_head=self.head, loader_binding=self.binding)
                with self.assertRaises(b.BundleError): b.validate_bound_bundle(out, source_raw=out, actual_head=h, loader_binding=self.binding)

    def test_header_blob_bounds_and_native_traversal_fail_closed(self):
        bad = []
        h = bytearray(self.head); h[64] ^= 1; bad.append(h)
        h = bytearray(self.head); off, _ = location(h, 64); h[off] ^= 1; bad.append(h)
        for offset, size in ((0x80, 34), (0x4000, 34), (0x3FE0, 34), (0xA0, 0), (0xA0, 33), (0xFFFFFFFF, 34)):
            h = bytearray(self.head); edit_entry(h, 64, lambda e, o=offset, n=size: struct.pack_into('<II', e, 4, o, n)); bad.append(h)
        h = bytearray(self.head); edit_entry(h, 32, lambda e: struct.pack_into('<H', e, 14, 1)); bad.append(h)
        h = bytearray(self.head); edit_entry(h, 64, lambda e: e.__setitem__(30, 255)); bad.append(h)
        bad.extend((self.head[:-1], self.head+b'\0'))
        for i, h in enumerate(bad):
            with self.subTest(case=i), self.assertRaises(b.BundleError): b.physical_head_key(bytes(h))

    def test_full_transport_and_atomic_persistence_before_finish(self):
        h = bytearray(self.head); h[0x100] ^= 1
        for bad in (None, 'model', 'nonce', 'data', 'key'):
            with self.subTest(bad=bad), tempfile.TemporaryDirectory() as td:
                dev = Device(); dev.head = bytes(h)
                if bad == 'key':
                    data = bytearray(dev.head); off, _ = location(data, 64); data[off] ^= 1; dev.head = bytes(data)
                dev.crc = zlib.crc32(dev.head); dev.bad = None if bad == 'key' else bad
                path = Path(td)/'head'; rows = []
                def exchange(req):
                    if struct.unpack_from('<I', req)[0] == host.FINISH:
                        self.assertEqual(path.read_bytes(), dev.head)
                        self.assertTrue(Path(str(path)+'.json').is_file())
                    return dev(req)
                call = lambda: host.capture(exchange, path, validate_head=lambda v: b.physical_head_key(v, diagnostics=rows.append), nonce=123)
                if bad is None:
                    self.assertEqual(call()['cleanup'], 'confirmed'); self.assertEqual(dev.finish, 1)
                    self.assertEqual(path.stat().st_mode & 0o777, 0o600); self.assertFalse(rows[0]['payloads'][0]['matches'])
                else:
                    with self.assertRaises(ValueError): call()
                    self.assertEqual(dev.finish, 0); self.assertEqual(list(Path(td).iterdir()), [])

    def test_fixed_smc_map_ignores_head_magic_generic_behavior_unchanged(self):
        h = bytearray(self.head); off, _ = location(h, 32)
        false_map = b.pm.slotmap_blob([(0x4000, 0x1000)])
        h[off+64:off+64+len(false_map)] = false_map
        out, _ = b.bind_head(self.raw, bytes(h), loader_binding=self.binding)
        product, logical = b.add_ctl.logical_of(out)
        self.assertEqual(fi.package_slot_map(logical, product), [(0x4000, 0x1000)])
        expected = b.fixed_slot_map()
        with patch.object(fi, 'slot_probe', return_value={}), patch.object(fi.xs, 'flash_map', return_value=expected), patch.object(fi.xs, 'at_risk', return_value=[]), patch.object(fi, 'package_slot_map', side_effect=AssertionError('whole-image lookup')):
            dev = NS(link=None, id=NS(text=b.PRODUCT))
            move = fi.plan_slots(dev, logical, product, require_backup=True, checked_package_map=expected)
            self.assertEqual(move.old_map, move.new_map)
            with self.assertRaisesRegex(AssertionError, 'whole-image'): fi.plan_slots(dev, logical, product, require_backup=True)

    def test_prepare_keeps_original_receipt_identity_with_diagnostic_separate(self):
        from smc_vendor_install_test import args
        with tempfile.TemporaryDirectory() as td:
            a = args(Path(td)); h = bytearray(self.head); h[0x100] ^= 1
            Path(a.head_ref).write_bytes(h); Path(a.package).write_bytes(self.raw)
            record = dict(schema=3, result='DEV CHECKED', profile=b.PROFILE, model='elite', config='dev', loader_binding=self.binding,
                          smc_vendor=b.validate_bundle(self.raw, loader_binding=self.binding))
            Path(a.dev_record).write_text(json.dumps(record))
            _, _, receipt, _ = si.prepare(a)
            self.assertEqual(set(receipt), {'schema','profile','source_sha256','converted_sha256','head_sha256','loader_binding','components','dev_record_sha256','source_commit'})
            self.assertFalse(a._smc_head_diagnostics['payloads'][0]['matches'])
            self.assertFalse(Path(a.smc_receipt).exists())

if __name__ == '__main__': unittest.main(verbosity=2)
