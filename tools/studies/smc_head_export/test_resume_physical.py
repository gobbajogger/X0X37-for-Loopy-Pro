# SPDX-License-Identifier: GPL-3.0-only
"""Synthetic resume/overlay provenance checks; no device or real exporter input."""
from pathlib import Path
from types import SimpleNamespace as NS
from unittest.mock import patch
import copy
import json
import struct
import sys
import tempfile
import unittest
import zlib
import resume_physical as r
import host
from test_host import Device
sys.path.insert(0, str(r.ROOT/'tests'))
from smc_vendor_bundle_test import BundleTest


def record_doc():
    return dict(schema=1, contract=r.CONTRACT, scope='resume-original-head-export-only',
                exporter=dict(source_sha256=r.exporter.SOURCE_SHA, ota_sha256=r.exporter.OTA_SHA),
                files={name:r.digest((r.ROOT/name).read_bytes()) for name in r.FILES},
                review=dict(status='independently-reviewed-host-only', reviewer='SYNTHETIC UNIT TEST, NOT ACTUAL REVIEW', evidence_sha256='a'*64))


class Test(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        BundleTest.setUpClass()

    def setUp(self):
        self.t = tempfile.TemporaryDirectory(); self.root = Path(self.t.name)
        self.output = self.root/'original-head'; self.receipt = self.root/'original-receipt'
        self.record = self.root/'host-overlay.json'; self.doc = record_doc()
        self.save_record()
        identity = dict(schema=1, profile='elite-head-export-dev', source_sha256=r.exporter.SOURCE_SHA,
                        ota_sha256=r.exporter.OTA_SHA, output=str(self.output.resolve()))
        self.receipt.write_text(json.dumps(identity)); self.original_receipt = self.receipt.read_bytes()

    def tearDown(self): self.t.cleanup()

    def save_record(self):
        self.record.write_text(json.dumps(self.doc, sort_keys=True))
        self.record_sha = r.digest(self.record.read_bytes())

    def test_exact_code_review_exporter_inventory(self):
        self.assertEqual(r.verify_overlay(self.record, self.record_sha), self.doc)
        with self.assertRaisesRegex(ValueError, 'record identity'): r.verify_overlay(self.record, '0'*64)
        mutations = []
        d = copy.deepcopy(self.doc); d['files'][r.FILES[0]] = '0'*64; mutations.append(d)
        d = copy.deepcopy(self.doc); del d['files'][r.FILES[0]]; mutations.append(d)
        d = copy.deepcopy(self.doc); d['exporter']['ota_sha256'] = '0'*64; mutations.append(d)
        d = copy.deepcopy(self.doc); d['review']['status'] = 'review-required'; mutations.append(d)
        d = copy.deepcopy(self.doc); d['review']['evidence_sha256'] = ''; mutations.append(d)
        original = self.doc
        for d in mutations:
            self.doc = d; self.save_record()
            with self.assertRaises(ValueError): r.verify_overlay(self.record, self.record_sha)
        self.doc = original; self.save_record()
        with patch.object(r.fi, '__file__', '/tmp/other-fm1-install.py'), self.assertRaisesRegex(ValueError, 'mixed overlay module'):
            r.verify_overlay(self.record, self.record_sha)

    def test_original_receipt_and_source_only(self):
        with patch.object(r.acquire, 'source', return_value=b'image'):
            r.original_transaction(b'raw', r.exporter.SOURCE_SHA, r.exporter.OTA_SHA, self.output, self.receipt)
            for source, ota, output, receipt in (
                ('0'*64, r.exporter.OTA_SHA, self.output, self.receipt),
                (r.exporter.SOURCE_SHA, '0'*64, self.output, self.receipt),
                (r.exporter.SOURCE_SHA, r.exporter.OTA_SHA, self.root/'moved-output', self.receipt),
                (r.exporter.SOURCE_SHA, r.exporter.OTA_SHA, self.output, self.root/'new-receipt'),
            ):
                with self.assertRaises(ValueError): r.original_transaction(b'raw', source, ota, output, receipt)
        with self.assertRaisesRegex(ValueError, 'source identity'):
            r.original_transaction(b'not actual original package', r.exporter.SOURCE_SHA, r.exporter.OTA_SHA, self.output, self.receipt)
        self.assertEqual(self.receipt.read_bytes(), self.original_receipt)

    def test_pending_loader_resume_saves_provenance_before_finish_and_never_stages(self):
        for mode in ('success', 'bad-key', 'normal-app', 'wrong-hello', 'cancel', 'provenance-save-failure'):
            with self.subTest(mode=mode):
                for p in (self.output, Path(str(self.output)+'.json'), Path(str(self.output)+'.validation.json')):
                    p.unlink(missing_ok=True)
                events = []; rows = []; dev = Device(); data = bytearray(BundleTest.other_head); data[0x100] ^= 1
                if mode == 'bad-key':
                    off = struct.unpack_from('<I', r.bundle._entry(data[64:96]), 4)[0]; data[off] ^= 1
                dev.head = bytes(data); dev.crc = zlib.crc32(dev.head)
                class Link:
                    def close(self): events.append('close')
                    def send(self, packet): events.append('admission-upgrade'); return True
                link = Link()
                device = NS(link=link, id=NS(loader=mode!='normal-app', text=r.bundle.PRODUCT), name='only')
                class Up:
                    def __init__(self, *_): pass
                    def find(self, *_): return device
                    def bind(self, *_): events.append('bind')
                    def check(self, *_): raise AssertionError('NEW STAGING FORBIDDEN')
                    def serve(self, *_): events.append('owned-admission'); return 1, True, False
                def exchange(link, req):
                    cmd, nonce, off, n = struct.unpack('<4I', req)
                    if cmd == 0:
                        events.append('hello')
                        return struct.pack('<7I', host.MAGIC, host.MODEL, nonce ^ (mode=='wrong-hello'), 0, 0, 0, 5)
                    if cmd == host.FINISH:
                        self.assertEqual(self.output.read_bytes(), dev.head)
                        meta = json.loads(Path(str(self.output)+'.json').read_text())
                        evidence = json.loads(Path(str(self.output)+'.validation.json').read_text())
                        self.assertEqual(meta['sha256'], evidence['head_sha256'])
                        self.assertEqual(evidence['validation']['overlay_record_sha256'], self.record_sha)
                    return dev(req)
                real_persist = host._persist
                def persist(path, data):
                    if mode == 'provenance-save-failure' and str(path).endswith('.validation.json'):
                        raise OSError('synthetic sidecar fsync failure')
                    return real_persist(path, data)
                with patch.object(r.acquire, 'source', return_value=b'image'), patch.object(r.fi, 'Updater', Up), patch.object(r.acquire, 'exchange', side_effect=exchange), patch.object(host, '_persist', side_effect=persist):
                    call = lambda: r.run(None, b'synthetic raw', r.exporter.SOURCE_SHA, r.exporter.OTA_SHA, self.output, self.receipt,
                                         overlay_record=self.record, overlay_sha256=self.record_sha,
                                         acknowledge=lambda: mode!='cancel', emit=rows.append)
                    if mode == 'success':
                        self.assertEqual(call()['cleanup'], 'confirmed')
                        self.assertEqual(dev.finish, 1)
                        self.assertFalse(json.loads(rows[0])['payloads'][0]['matches'])
                    else:
                        with self.assertRaises((ValueError, OSError)): call()
                        self.assertEqual(dev.finish, 0)
                        if mode != 'provenance-save-failure': self.assertFalse(self.output.exists())
                        if mode in ('normal-app', 'wrong-hello', 'cancel'): self.assertNotIn('admission-upgrade', events)
                self.assertEqual(self.receipt.read_bytes(), self.original_receipt)


if __name__ == '__main__': unittest.main(verbosity=2)
