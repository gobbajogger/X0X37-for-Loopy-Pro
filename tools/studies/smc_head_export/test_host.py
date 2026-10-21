# SPDX-License-Identifier: GPL-3.0-only
import os
from pathlib import Path
import struct
import tempfile
import unittest
import zlib
import host

class Device:
    def __init__(self):
        self.head=bytes(range(256))*64;self.crc=zlib.crc32(self.head);self.finish=0;self.reads=0;self.bad=None
    def __call__(self,req):
        cmd,nonce,off,n=struct.unpack('<4I',req)
        if cmd==host.INFO:self.nonce=nonce;off=host.SIZE
        if cmd==host.READ:
            self.reads+=1
            if self.bad=='timeout' and self.reads==3:raise TimeoutError('partial transfer')
        if cmd==host.FINISH:
            self.finish+=1
            if self.bad=='finish':raise TimeoutError('cleanup acknowledgment lost')
        data=self.head[off:off+n] if cmd==host.READ else b''
        if self.bad=='data' and cmd==host.READ and off==224:data=bytes([data[0]^1])+data[1:]
        model=host.MODEL if self.bad!='model' else host.MODEL^1
        if self.bad=='nonce':nonce^=1
        return struct.pack('<7I',host.MAGIC,model,nonce,self.crc,off,len(data),host.DONE if cmd==host.FINISH else 0)+data

class Test(unittest.TestCase):
    def setUp(self):self.t=tempfile.TemporaryDirectory();self.p=Path(self.t.name)/'unit-head.bin';self.d=Device()
    def tearDown(self):self.t.cleanup()
    def capture(self):return host.capture(self.d,self.p,validate_head=lambda b:len(b)==16384,nonce=123)
    def test_atomic_complete_before_finish(self):
        dev=self.d
        def exchange(req):
            if struct.unpack_from('<I',req)[0]==host.FINISH:
                self.assertEqual(self.p.read_bytes(),dev.head);self.assertTrue(self.p.with_name(self.p.name+'.json').exists())
            return dev(req)
        r=host.capture(exchange,self.p,validate_head=lambda b:None,nonce=123)
        self.assertEqual(r['cleanup'],'confirmed');self.assertEqual(self.p.stat().st_mode&0o777,0o600)
        self.assertEqual(self.capture()['cleanup'],'confirmed') # identical saved file is retained
    def test_partial_and_restart(self):
        self.d.bad='timeout'
        with self.assertRaises(TimeoutError):self.capture()
        self.assertFalse(self.p.exists());self.assertFalse(self.d.finish)
        self.d.bad=None
        self.assertEqual(host.capture(self.d,self.p,validate_head=lambda b:None,nonce=124)['cleanup'],'confirmed')
    def test_model_nonce_crc_rejected_without_cleanup(self):
        for bad in ('model','nonce','data'):
            self.d=Device();self.d.bad=bad
            with self.subTest(bad=bad),self.assertRaises(host.ExportError):self.capture()
            self.assertFalse(self.p.exists());self.assertFalse(self.d.finish)
    def test_lost_finish_keeps_verified_file(self):
        self.d.bad='finish';r=self.capture()
        self.assertEqual(r['cleanup'],'uncertain');self.assertTrue(r['saved']);self.assertEqual(self.d.finish,3)
        self.assertEqual(self.p.read_bytes(),self.d.head)
    def test_no_overwrite_or_alias(self):
        self.p.write_bytes(b'other unit')
        with self.assertRaises(host.ExportError):self.capture()
        self.assertEqual(self.p.read_bytes(),b'other unit');self.assertFalse(self.d.finish)
        with self.assertRaises(host.ExportError):host.capture(self.d,self.p,validate_head=lambda b:None,protected_paths=[self.p])
        alias=self.p.with_name('alias');os.link(self.p,alias)
        with self.assertRaises(host.ExportError):host.capture(self.d,self.p,validate_head=lambda b:None,protected_paths=[alias])

if __name__=='__main__':unittest.main()
