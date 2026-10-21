#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host-only adapter checks; no MIDI backend or device is opened."""
import copy
import io
import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace as NS
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
sys.path.insert(0,str(Path(__file__).resolve().parent))
import fm1_install as fi
import smc_vendor_install as si
import smc_vendor_bundle as b
from smc_vendor_bundle_test import BundleTest


def args(root):
    return NS(config='dev',dev_record=str(root/'record.json'),commit='a'*40,ctl=None,no_ctl=False,force=False,no_slots=False,
              backup=str(root/'backup.x0xslots'),verify_slots=True,smc_receipt=str(root/'receipt.json'),smc_cold_recovery=True,
              head_ref=str(root/'head.bin'),package=str(root/'package.fwsc'),smc_vendor_dev=True,smc_logical_rollback=False,
              port=None,yes=True)


class InstallTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):BundleTest.setUpClass()

    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name);self.a=args(self.root)
        self.meta=b.validate_bundle(BundleTest.raw,loader_binding=BundleTest.binding)
        self.record=dict(schema=3,result='DEV CHECKED',profile=b.PROFILE,model='elite',config='dev',loader_binding=BundleTest.binding,smc_vendor=self.meta)
        Path(self.a.package).write_bytes(BundleTest.raw);Path(self.a.head_ref).write_bytes(BundleTest.other_head)
        Path(self.a.dev_record).write_text(json.dumps(self.record))
    def tearDown(self):self.temp.cleanup()

    def test_prepare_freezes_identity(self):
        product,image,receipt,meta=si.prepare(self.a)
        self.assertEqual(product,b.PRODUCT);self.assertIs(type(image),bytes)
        self.assertEqual(meta,self.meta)
        self.assertEqual(receipt['head_sha256'],b.sha(BundleTest.other_head))
        Path(self.a.package).write_bytes(b'changed path after admission')
        self.assertEqual(si.prepare(self.a)[1],image)

    def test_bypass_and_record_negatives(self):
        for k,v in [('config','rel'),('ctl','some.img'),('no_ctl',True),('force',True),('no_slots',True),('backup',None),('verify_slots',False),('smc_receipt',None),('smc_cold_recovery',False),('head_ref',None)]:
            a=copy.copy(self.a);setattr(a,k,v)
            with self.subTest(k=k),self.assertRaises(b.BundleError):si.prepare(a)
        self.record['smc_vendor']['canonical_main_sha256']='0'*64
        Path(self.a.dev_record).write_text(json.dumps(self.record))
        with self.assertRaises(b.BundleError):si.prepare(self.a)

    def test_record_swap_and_wrong_policy(self):
        self.a._smc_record_sha='0'*64
        with self.assertRaises(b.BundleError):si.prepare(self.a)
        del self.a._smc_record_sha
        self.a.smc_logical_rollback=True
        with self.assertRaises(b.BundleError):si.prepare(self.a)

    def test_exact_region_echo_and_length(self):
        wanted=bytes(range(256))*2
        def req(ed,cmd,a,*_):
            self.assertEqual(cmd,39);off=a[1]|a[2]<<7|a[3]<<14
            return a[:4]+fi.xs.pack7(wanted[off:off+min(224,a[4]*2)])
        with patch.object(fi.xs,'req',req):self.assertEqual(si.read_region(None,2,len(wanted),fi),wanted)
        for bad in ([3,0,0,0], [2,0,0,0]+fi.xs.pack7(b'bad'), [2,0,0,0,128]):
            with patch.object(fi.xs,'req',return_value=bad),self.assertRaises(fi.InstallError):si.read_region(None,2,224,fi)

    def test_wrong_finish_never_acknowledged(self):
        class Link:
            lost=False
            def __init__(self):self.sent=[]
            def read(self,_):return b'packet'
            def send(self,p):self.sent.append(p);return True
        up=fi.Updater(None);up.read_policy=b.response_bytes
        for req in ((0,fi.FINISH_WRITE,8),(0,fi.FINISH_CHECK,7),(1,0,1),(0,3,1)):
            link=Link()
            with patch.object(fi,'parse_request',return_value=req),self.assertRaises(fi.InstallError):
                up.serve(link,b'abc',fi.FINISH_CHECK,0,lambda *_:None)
            self.assertEqual(link.sent,[])
        link=Link()
        with patch.object(fi,'parse_request',return_value=(0,fi.FINISH_CHECK,8)):
            self.assertEqual(up.serve(link,b'abc',fi.FINISH_CHECK,0,lambda *_:None),(0,True,False))
        self.assertEqual(len(link.sent),1)

    def test_transaction_and_resume_binding(self):
        calls=[];mapping=[(0xC2000,0x14000),(0xD6000,0x1E000),(0xB0000,0xB000)]
        class Link:
            def close(self):pass
        class Move:
            def __init__(self,*_):
                self.old_map=self.new_map=mapping;self.risk=[];self.lay={'count':3};self.images=[None]*3
            def backup(self,link,path,out):Path(path).write_bytes(b'backup');calls.append('backup')
            def verify(self,link,out):calls.append('slots-read')
        class Up:
            loader=False;model=b.PRODUCT
            def __init__(self,*_):pass
            def find(self,*_):return NS(id=NS(loader=self.loader,text=self.model),link=Link(),name='only')
            def bind(self,d):pass
            def install(self,d,image,product,progress):
                self_test.assertTrue(Path(self_test.a.smc_receipt).exists());calls.append('install')
            def wait_for(self,*_):return NS(id=NS(loader=False,text=b.PRODUCT),link=Link(),name='only')
        self_test=self
        with patch.object(fi,'Updater',Up),patch.object(fi,'plan_slots',return_value=Move()),patch.object(fi,'SlotMove',Move),patch.object(fi.xs,'read_archive',return_value=({},[None]*3)),patch.object(si,'check_regions',side_effect=lambda *_:calls.append('regions-read')):
            self.assertEqual(si.run(self.a,None,io.StringIO(),lambda _:True,fi),0)
            self.assertEqual(calls,['backup','install','regions-read','slots-read'])
            # Written/verified receipt: only verification, no duplicate update.
            calls.clear();self.assertEqual(si.run(self.a,None,io.StringIO(),lambda _:True,fi),0)
            self.assertEqual(calls,['regions-read','slots-read'])
            # Interrupted writer: same package and saved backup permit resume.
            t=json.loads(Path(self.a.smc_receipt).read_text());t['state']='writing';si.save_receipt(self.a.smc_receipt,t)
            Up.loader=True;calls.clear();self.assertEqual(si.run(self.a,None,io.StringIO(),lambda _:True,fi),0)
            self.assertEqual(calls,['install','regions-read','slots-read'])
            Path(self.a.backup).write_bytes(b'changed')
            with self.assertRaises(fi.InstallError):si.run(self.a,None,io.StringIO(),lambda _:True,fi)
            self.assertEqual(calls,['install','regions-read','slots-read'])

if __name__=='__main__':unittest.main()
