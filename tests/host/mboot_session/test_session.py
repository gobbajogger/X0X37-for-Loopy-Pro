#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""No device backend. Synthetic C provider -> packed frames -> real backup core."""
import ctypes as C
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'tools/mboot'))
import capture_host as h
import session_host as w

class SessionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory(prefix='mboot-session-')
        lib=Path(cls.temp.name)/'session.so'
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',
            '-I'+str(ROOT/'tools/mboot'),str(ROOT/'tools/mboot/capture_core.c'),
            str(ROOT/'tools/mboot/session_wire.c'),str(Path(__file__).with_name('fixture.c')),
            '-o',str(lib)],check=True)
        cls.lib=C.CDLL(str(lib))
        cls.lib.fixture_exchange.argtypes=[C.c_void_p,C.c_size_t,C.c_void_p,C.c_size_t]
        cls.lib.fixture_exchange.restype=C.c_size_t
        cls.lib.fixture_count.argtypes=[C.c_uint]
        cls.lib.fixture_count.restype=C.c_uint
    @classmethod
    def tearDownClass(cls): cls.temp.cleanup()
    def setUp(self): self.lib.fixture_reset()
    def raw(self,q):
        out=C.create_string_buffer(383);inp=C.create_string_buffer(q)
        n=self.lib.fixture_exchange(inp,len(q),out,len(out));return out.raw[:n]
    def exchange(self,frame):
        self.assertEqual(frame[:5],b'\xf0\x7d\x46\x4c\x31')
        q=w.unpack7(frame[5:-1]);return frame[:5]+w.pack7(self.raw(q))+b'\xf7'
    def transport(self,exchange=None,unit=b'\x01'*32):
        return w.WireTransport(exchange or self.exchange,expected_unit=unit,model=2,selection=b'\x04'*32)
    def request(self,op,seq=0,payload=b''):
        return self.raw(struct.pack('<BBI',1,op,seq)+payload)
    def test_end_to_end_private_backup(self):
        t=self.transport()
        with tempfile.TemporaryDirectory() as parent,h.Core() as core:
            receipt=h.retain_capture(t,core,Path(parent),'backup')
            data=(Path(parent)/'backup/image.bin').read_bytes()
            self.assertEqual(len(data),h.SIZE)
            self.assertEqual(data,bytes(i%251 for i in range(h.SIZE)))
            self.assertEqual(receipt['sha256'],hashlib.sha256(data).hexdigest())
            self.assertFalse(receipt['lease_authority'])
            self.assertFalse(receipt['device_identity_verified_by_this_adapter'])
            self.assertEqual(os.stat(Path(parent)/'backup/image.bin').st_mode&0o777,0o400)
        self.assertEqual(self.lib.fixture_count(3),0)
        self.assertEqual(self.lib.fixture_count(1),1)
    def test_default_unsupported(self):
        self.assertFalse(w.WireTransport().supported)
        self.lib.fixture_mode(6);t=self.transport();self.assertFalse(t.supported)
        with self.assertRaises(h.CaptureError):t.acquire()
        self.assertEqual(self.lib.fixture_count(2),0)
    def test_pack_boundaries(self):
        for n in range(1,384):
            data=bytes(i%256 for i in range(n));self.assertEqual(w.unpack7(w.pack7(data)),data)
    def test_noncanonical_pack(self):
        for data in (b'',b'\x00',b'\x80\x00',b'\x02\x00',b'\x00\x80'):
            with self.assertRaises(h.CaptureError):w.unpack7(data)
    def test_foreign_owner_preserves_lease(self):
        t=self.transport();b,*_=t.acquire()
        response=self.request(w.CLOSE,1,b'\x09'*32)
        self.assertEqual(response[6],4);self.assertEqual(self.lib.fixture_count(3),1)
        t.release(b);self.assertEqual(self.lib.fixture_count(3),0)
    def test_stale_sequence_before_read(self):
        t=self.transport();b,*_=t.acquire()
        response=self.request(w.READ,8,b.session+struct.pack('<IH',0,1))
        self.assertNotEqual(response[6],0);self.assertEqual(self.lib.fixture_count(0),0)
        self.assertEqual(self.lib.fixture_count(3),0)
    def test_bad_geometry_before_read(self):
        for off,n in ((1,1),(0,0),(0,225),(0xffffffff,1),(h.SIZE,1)):
            self.lib.fixture_reset();t=self.transport();b,*_=t.acquire()
            response=self.request(w.READ,1,b.session+struct.pack('<IH',off,n))
            self.assertNotEqual(response[6],0);self.assertEqual(self.lib.fixture_count(0),0)
    def test_active_open_does_not_replace_owner(self):
        t=self.transport();b,*_=t.acquire()
        self.assertEqual(self.request(w.OPEN,0,b'\x03'*32+b'\x04'*32)[6],3)
        self.assertEqual(self.lib.fixture_count(2),1);t.release(b)
    def test_bad_qualification_released(self):
        t=self.transport();self.lib.fixture_mode(1)
        with self.assertRaises(h.CaptureError):t.acquire()
        self.assertEqual(self.lib.fixture_count(3),0)
    def test_actual_unit_mismatch_retains_close_token(self):
        t=self.transport(unit=b'\x09'*32)
        with self.assertRaises(h.CaptureError):t.acquire()
        t.release(None);self.assertEqual(self.lib.fixture_count(3),0)
    def test_identity_change_before_read(self):
        t=self.transport();t.acquire();self.lib.fixture_mode(2)
        with self.assertRaises(h.CaptureError):next(t.chunks())
        self.assertEqual(self.lib.fixture_count(0),0);self.assertEqual(self.lib.fixture_count(3),0)
    def test_identity_change_after_read_hides_data(self):
        t=self.transport();b,*_=t.acquire();self.lib.fixture_mode(7)
        response=self.request(w.READ,1,b.session+struct.pack('<IH',0,224))
        self.assertEqual(len(response),7);self.assertNotEqual(response[6],0)
    def test_idle_expiry_releases(self):
        t=self.transport();t.acquire();self.lib.fixture_mode(3)
        self.assertNotEqual(self.lib.fixture_tick(),0);self.assertEqual(self.lib.fixture_count(3),0)
    def test_release_failure_quarantines(self):
        t=self.transport();b,*_=t.acquire();self.lib.fixture_mode(5)
        with self.assertRaises(h.CaptureError):t.release(b)
        self.assertEqual(self.lib.fixture_count(3),1)
        self.assertEqual(self.request(w.OPEN,0,b'\x03'*32+b'\x04'*32)[6],3)
        self.lib.fixture_mode(0);t.release(b);self.assertEqual(self.lib.fixture_count(3),0)
    def test_lost_open_never_claims_release(self):
        def lose(frame):
            response=self.exchange(frame)
            if w.unpack7(frame[5:-1])[1]==w.OPEN:raise h.CaptureError('PRIVATE MARKER')
            return response
        t=self.transport(lose)
        with self.assertRaisesRegex(h.CaptureError,'packet exchange failed'):t.acquire()
        with self.assertRaisesRegex(h.CaptureError,'outcome unresolved'):t.release(None)
        self.assertEqual(self.lib.fixture_count(3),1)
        self.lib.fixture_mode(3);self.lib.fixture_tick();self.assertEqual(self.lib.fixture_count(3),0)
    def test_lost_close_reply_reconciles_same_owner(self):
        lost = False
        def lose_once(frame):
            nonlocal lost
            response=self.exchange(frame)
            if w.unpack7(frame[5:-1])[1]==w.CLOSE and not lost:
                lost=True
                raise h.CaptureError('PRIVATE MARKER')
            return response
        t=self.transport(lose_once);b,*_=t.acquire()
        with self.assertRaises(h.CaptureError):t.release(b)
        self.assertTrue(t.pending);self.assertEqual(self.lib.fixture_count(3),0)
        t.release(b)
        self.assertFalse(t.pending);self.assertEqual(self.lib.fixture_count(1),1)
    def test_auto_release_cleanup_is_reconciled(self):
        t=self.transport()
        with tempfile.TemporaryDirectory() as parent,h.Core() as core:
            self.lib.fixture_mode(4)
            with self.assertRaises(h.CaptureError) as caught:
                h.retain_capture(t,core,Path(parent),'backup')
            self.assertNotIn('release unresolved',str(caught.exception))
            self.assertFalse((Path(parent)/'backup').exists())
        self.assertFalse(t.pending);self.assertEqual(self.lib.fixture_count(3),0)
        self.assertEqual(self.lib.fixture_count(1),1)
    def test_two_complete_cycles_same_transport(self):
        t=self.transport()
        with tempfile.TemporaryDirectory() as parent,h.Core() as core:
            for name in ('first','second'):
                receipt=h.retain_capture(t,core,Path(parent),name)
                self.assertEqual(receipt['size'],h.SIZE)
                self.assertFalse(t.pending)
        self.assertEqual(self.lib.fixture_count(2),2)
        self.assertEqual(self.lib.fixture_count(1),2)
    def test_closed_tombstone_never_releases_active_foreign_owner(self):
        t=self.transport();b,*_=t.acquire();t.release(b)
        self.assertEqual(self.request(w.CLOSE,99,b.session)[6],0)
        # A different token must never get a clean-close acknowledgment.
        self.assertEqual(self.request(w.CLOSE,99,b'\x09'*32)[6],4)
        self.assertEqual(self.lib.fixture_count(1),1)
        t.acquire()
        self.assertNotEqual(t.binding.session,b.session)
        self.assertEqual(self.request(w.CLOSE,99,b.session)[6],4)
        self.assertEqual(self.lib.fixture_count(3),1)
        t.release(t.binding)
    def test_transport_exception_sanitized(self):
        def fail(_):raise h.CaptureError('PRIVATE MARKER')
        with self.assertRaisesRegex(h.CaptureError,'packet exchange failed'):self.transport(fail)
    def test_reply_sequence_and_size_checked(self):
        for change in (lambda r:r+b'\x00',lambda r:r[:5]+w.pack7(b'\x01\x00\x01\x00\x00\x00\x00')+b'\xf7'):
            with self.assertRaises(h.CaptureError):self.transport(lambda q:change(self.exchange(q)))
    def test_finish_before_complete_rejected(self):
        t=self.transport();b,*_=t.acquire()
        self.assertNotEqual(self.request(w.FINISH,1,b.session+bytes(64))[6],0)
        self.assertEqual(self.lib.fixture_count(3),0)
    def test_unknown_version_and_opcode(self):
        for q in (b'\x02\x00'+bytes(4),b'\x01\x07'+bytes(4)):
            self.assertEqual(self.raw(q)[6],2)
        self.assertEqual(self.lib.fixture_count(2),0)

if __name__=='__main__':unittest.main()
