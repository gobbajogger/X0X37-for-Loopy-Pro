#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Temporary deterministic synthetic backups only; no device/real owner data."""
from dataclasses import replace
import hashlib
import json
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'tools/mboot'))
import capture_host as h

DATA=bytes(range(256))*(h.SIZE//256)
FULL,HEAD=hashlib.sha256(DATA).digest(),hashlib.sha256(DATA[:h.HEAD]).digest()
B=h.Binding(bytes([1])*32,bytes([2])*32,bytes([3])*32,bytes([4])*32,7,2)


class Synthetic:
    supported=True
    def __init__(self,fault=None):self.fault=fault;self.released=0;self.acquired=0
    def acquire(self):
        self.acquired+=1
        if self.fault=='acquire-malformed':return (B,)
        if self.fault=='acquire-error':raise RuntimeError('private-data-must-not-escape')
        return B,0 if self.fault=='caps' else 15,1,100
    def chunks(self):
        for off in range(0,h.SIZE,h.CHUNK):
            binding=B; data=DATA[off:off+h.CHUNK]; now=2
            if off==224:
                if self.fault=='unit':binding=replace(B,unit=bytes([9])*32)
                if self.fault=='epoch':binding=replace(B,epoch=8)
                if self.fault=='gap':off+=1
                if self.fault=='duplicate':off=0
                if self.fault=='bool-offset':off=True
                if self.fault=='oversize':data+=b'x'
                if self.fault=='mutable':data=bytearray(data)
                if self.fault=='expiry':now=100
                if self.fault=='exception':raise RuntimeError('private-data-must-not-escape')
                if self.fault=='partial':return
                if self.fault=='envelope':yield {};return
            yield binding,off,data,now
    def finish(self):
        if self.fault=='finish-malformed':return (B,)
        return B,bytes(32) if self.fault=='digest' else FULL,HEAD,3
    def revalidate(self):
        return B,B.challenge if self.fault=='replay' else bytes([8])*32,FULL,HEAD,4
    def current(self):
        return (replace(B,selection=bytes([5])*32) if self.fault=='reselect' else B,
                100 if self.fault=='late-expiry' else 5)
    def release(self,binding):
        self.released+=1
        if self.fault=='release':raise RuntimeError('private-data-must-not-escape')


class Host(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.core=h.Core().__enter__()
    @classmethod
    def tearDownClass(cls):cls.core.__exit__()
    def setUp(self):self.tmp=tempfile.TemporaryDirectory(prefix='synthetic-mboot-backup-');self.parent=Path(self.tmp.name)
    def tearDown(self):self.tmp.cleanup()
    def capture(self,t=None,name='synthetic'):
        return h.retain_capture(t or Synthetic(),self.core,self.parent,name)
    def test_complete_private_backup_and_offline_verifier(self):
        t=Synthetic();result=self.capture(t);d=self.parent/'synthetic'
        self.assertEqual(t.released,1);self.assertEqual(result['status'],h.STATUS)
        self.assertEqual((d/'image.bin').read_bytes(),DATA)
        self.assertEqual(stat.S_IMODE(d.stat().st_mode),0o700)
        for n in ['image.bin','receipt.json']:self.assertEqual(stat.S_IMODE((d/n).stat().st_mode),0o400)
        self.assertEqual(set(p.name for p in d.iterdir()),{'image.bin','receipt.json'})
        verified=h.verify_backup(d)
        self.assertEqual(verified['status'],'OFFLINE_BACKUP_CONSISTENT_NOT_AUTHORIZATION')
        self.assertNotIn('sha256',verified)
    def test_transport_captureerror_is_sanitized_at_every_boundary(self):
        marker='PRIVATE-UNIT-PAYLOAD-TEST-MARKER'
        for method in ['acquire','chunks','finish','revalidate','current']:
            with self.subTest(method=method):
                t=Synthetic()
                def failing():raise h.CaptureError(marker)
                if method=='chunks':
                    def failing_iterator():
                        yield B,0,DATA[:h.CHUNK],2
                        raise h.CaptureError(marker)
                    t.chunks=failing_iterator
                else:setattr(t,method,failing)
                with self.assertRaises(h.CaptureError) as error:self.capture(t)
                self.assertNotIn(marker,str(error.exception))
                self.assertEqual(t.released,1);self.assertEqual(list(self.parent.iterdir()),[])
        class BadCapability:
            @property
            def supported(self):raise h.CaptureError(marker)
        with self.assertRaises(h.CaptureError) as error:self.capture(BadCapability())
        self.assertNotIn(marker,str(error.exception))
    def test_existing_active_core_survives_failed_start(self):
        b=h.cbinding(B);self.core.call('start',h.C.byref(b),15,1,100)
        try:
            with self.assertRaisesRegex(h.CaptureError,'refused start'):self.capture()
            data=h.C.create_string_buffer(DATA[:h.CHUNK])
            self.core.call('chunk',h.C.byref(b),0,data,h.CHUNK,2)
        finally:self.core.lib.mb_capture_invalidate(self.core.ctx)
    def test_existing_active_core_survives_prestart_failure(self):
        b=h.cbinding(B);self.core.call('start',h.C.byref(b),15,1,100)
        (self.parent/'synthetic').mkdir()
        try:
            with self.assertRaises(h.CaptureError):self.capture()
            data=h.C.create_string_buffer(DATA[:h.CHUNK])
            self.core.call('chunk',h.C.byref(b),0,data,h.CHUNK,2)
        finally:self.core.lib.mb_capture_invalidate(self.core.ctx)
    def test_live_default_refused_before_files_or_acquisition(self):
        with self.assertRaises(h.CaptureError):self.capture(h.UnsupportedTransport())
        self.assertEqual(list(self.parent.iterdir()),[])
    def test_protocol_negatives_cleanup_and_release(self):
        for fault in ['caps','unit','epoch','gap','duplicate','bool-offset','oversize','mutable','expiry',
                      'exception','partial','envelope','finish-malformed','digest','replay','reselect','late-expiry']:
            with self.subTest(fault=fault):
                t=Synthetic(fault)
                with self.assertRaises(h.CaptureError) as error:self.capture(t)
                self.assertNotIn('private-data',str(error.exception))
                self.assertEqual(t.released,1);self.assertEqual(list(self.parent.iterdir()),[])
    def test_provisional_acquire_failure_released(self):
        for fault in ['acquire-malformed','acquire-error']:
            t=Synthetic(fault)
            with self.assertRaises(h.CaptureError):self.capture(t)
            self.assertEqual(t.released,1);self.assertEqual(list(self.parent.iterdir()),[])
    def test_release_failure_retains_backup_but_no_success(self):
        t=Synthetic('release')
        with self.assertRaisesRegex(h.CaptureError,'release unresolved'):self.capture(t)
        self.assertEqual(h.verify_backup(self.parent/'synthetic')['size'],h.SIZE)
    def test_destination_exists_never_overwritten_or_acquired(self):
        d=self.parent/'synthetic';d.mkdir();(d/'sentinel').write_text('untouched');t=Synthetic()
        with self.assertRaises(h.CaptureError):self.capture(t)
        self.assertEqual(t.acquired,0);self.assertEqual((d/'sentinel').read_text(),'untouched')
    def test_child_symlink_and_unsafe_name_refused(self):
        target=self.parent/'target';target.mkdir();(self.parent/'synthetic').symlink_to(target)
        with self.assertRaises(h.CaptureError):self.capture()
        self.assertTrue((self.parent/'synthetic').is_symlink())
        for name in ['../escape','a/b','','UPPER']:
            with self.assertRaises(h.CaptureError):self.capture(name=name)
    def test_public_or_symlink_parent_refused(self):
        os.chmod(self.parent,0o755)
        with self.assertRaises(h.CaptureError):self.capture()
        os.chmod(self.parent,0o700);link=self.parent/'link';private=self.parent/'private';private.mkdir(mode=0o700);link.symlink_to(private)
        with self.assertRaises(h.CaptureError):h.retain_capture(Synthetic(),self.core,link,'synthetic')
    def test_short_writes_are_completed(self):
        original=os.write
        with patch.object(h.os,'write',side_effect=lambda fd,data:original(fd,data[:max(1,len(data)//2)])):
            self.capture()
        self.assertEqual(h.verify_backup(self.parent/'synthetic')['size'],h.SIZE)
    def test_write_failure_cleanup(self):
        t=Synthetic()
        with patch.object(h.os,'write',side_effect=OSError('disk-full')):
            with self.assertRaises(h.CaptureError):self.capture(t)
        self.assertEqual(t.released,1);self.assertEqual(list(self.parent.iterdir()),[])
    def test_stored_byte_corruption_refused_before_publication(self):
        original=os.write;t=Synthetic()
        def damaged(fd,data):
            raw=bytes(data)
            return original(fd,bytes([raw[0]^1])+raw[1:])
        with patch.object(h.os,'write',side_effect=damaged):
            with self.assertRaisesRegex(h.CaptureError,'retained backup'):self.capture(t)
        self.assertEqual(t.released,1);self.assertEqual(list(self.parent.iterdir()),[])
    def test_fsync_failure_cleanup(self):
        original=os.fsync;calls=0;t=Synthetic()
        def fail_later(fd):
            nonlocal calls
            calls+=1
            if calls==2:raise OSError('sync-failed')
            return original(fd)
        with patch.object(h.os,'fsync',side_effect=fail_later):
            with self.assertRaises(h.CaptureError):self.capture(t)
        self.assertEqual(t.released,1);self.assertEqual(list(self.parent.iterdir()),[])
    def test_publication_collision_refused_no_overwrite(self):
        original=os.link
        def collision(src,dst,**kwargs):
            if dst=='receipt.json':raise FileExistsError('simulated collision')
            return original(src,dst,**kwargs)
        with patch.object(h.os,'link',side_effect=collision):
            with self.assertRaises(h.CaptureError):self.capture()
        self.assertEqual(list(self.parent.iterdir()),[])
    def test_collision_preserves_unowned_file(self):
        original=os.link
        def collision(src,dst,**kwargs):
            if dst=='receipt.json':
                fd=os.open(dst,os.O_CREAT|os.O_EXCL|os.O_WRONLY,0o600,dir_fd=kwargs['dst_dir_fd'])
                os.write(fd,b'unknown-owner-file');os.close(fd)
            return original(src,dst,**kwargs)
        with patch.object(h.os,'link',side_effect=collision):
            with self.assertRaises(h.CaptureError):self.capture()
        d=self.parent/'synthetic'
        self.assertEqual((d/'receipt.json').read_bytes(),b'unknown-owner-file')
        self.assertEqual([p.name for p in d.iterdir()],['receipt.json'])
    def test_parent_sync_failure_does_not_acquire_or_leave_directory(self):
        t=Synthetic()
        with patch.object(h.os,'fsync',side_effect=OSError('parent-sync')):
            with self.assertRaises(h.CaptureError):self.capture(t)
        self.assertEqual(t.acquired,0);self.assertEqual(list(self.parent.iterdir()),[])
    def test_verifier_refuses_fifo_and_hardlink(self):
        self.capture();d=self.parent/'synthetic';p=d/'image.bin';p.unlink();os.mkfifo(p,0o600)
        with self.assertRaises(h.CaptureError):h.verify_backup(d)
        p.unlink();p.write_bytes(DATA);os.chmod(p,0o400);os.link(p,d/'other')
        with self.assertRaises(h.CaptureError):h.verify_backup(d)
    def test_verifier_corruption_truncation_and_permissions(self):
        self.capture();p=self.parent/'synthetic/image.bin';os.chmod(p,0o600);p.write_bytes(DATA[:-1])
        with self.assertRaises(h.CaptureError):h.verify_backup(p.parent)
        p.write_bytes(DATA);os.chmod(p,0o644)
        with self.assertRaises(h.CaptureError):h.verify_backup(p.parent)
    def test_verifier_no_symlink_follow(self):
        self.capture();d=self.parent/'synthetic';p=d/'image.bin';p.unlink();p.symlink_to(self.parent/'absent')
        with self.assertRaises(OSError):h.verify_backup(d)
    def test_duplicate_receipt_and_authority_forgery_refused(self):
        self.capture();d=self.parent/'synthetic';p=d/'receipt.json';os.chmod(p,0o600);good=p.read_text()
        p.write_text(good[:-2]+',"schema":1}\n')
        with self.assertRaises(h.CaptureError):h.verify_backup(d)
        r=json.loads(good);r['lease_authority']=True;p.write_text(json.dumps(r))
        with self.assertRaises(h.CaptureError):h.verify_backup(d)
    def test_partial_residue_not_complete(self):
        d=self.parent/'partial';d.mkdir(mode=0o700);(d/'image.partial').write_bytes(b'fake')
        with self.assertRaises(OSError):h.verify_backup(d)
    def test_cli_status_and_local_verify_sanitized(self):
        script=ROOT/'tools/mboot/capture_host.py'
        r=subprocess.run([sys.executable,'-B',str(script)],text=True,capture_output=True,check=True)
        self.assertEqual(json.loads(r.stdout)['live_capture'],'UNSUPPORTED')
        self.capture();r=subprocess.run([sys.executable,'-B',str(script),'--verify-backup',str(self.parent/'synthetic')],text=True,capture_output=True,check=True)
        self.assertNotIn(FULL.hex(),r.stdout);self.assertNotIn(B.unit.hex(),r.stdout)
        self.assertIn('NOT_AUTHORIZATION',r.stdout)


if __name__=='__main__':unittest.main(verbosity=2)
