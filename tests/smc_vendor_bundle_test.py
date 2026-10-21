#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Synthetic source-only tests; optional actual components are read, never saved.
  python3 tests/smc_vendor_bundle_test.py
  python3 tests/smc_vendor_bundle_test.py COMPONENTS_ROOT SDK_ROOT
"""
import copy
import json
from pathlib import Path
import struct
import sys
import unittest
import zlib
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import smc_vendor_bundle as b


def synthetic():
    ui = bytes(8192)
    app = bytearray(4096)
    struct.pack_into('<5I',app,64,0x55493058,1,0xAB000,8192,zlib.crc32(ui))
    regions = [bytearray(b'\xff'*n) for _,n in b.REGIONS]
    h = [0x53423058,1,128,4,b.MODEL_ID,0x534D4302,1,123,
         0x02080080,0x02080082,0x01C01800,0x01C02000,0x01C01800,0x01C03000,
         0x01C7D400,0x01C7D400,0,0]
    for i,(off,n) in enumerate(b.REGIONS):
        used = 256
        h.extend([off,used,used if i==0 else 0,zlib.crc32(regions[i][128 if i==0 else 0:used])])
    h.extend([0,0,0]);raw=struct.pack('<IHH30I',*h);h[30]=zlib.crc32(raw)
    regions[0][:128]=struct.pack('<IHH30I',*h)
    ota=bytearray(1024)
    struct.pack_into('<II',ota,4,32,len(ota)-32)
    struct.pack_into('<II',ota,36,4096,0x01C0A800)
    ota[0x48:0x58]=b'X0X-BUNDLE-LDR1\0'
    struct.pack_into('<H',ota,32,b.pm.crc16(ota[34:64]))
    struct.pack_into('<H',ota,2,b.pm.crc16(ota[32:]))
    struct.pack_into('<H',ota,0,b.pm.crc16(ota[2:32]))
    binding=dict(app_size=len(app),app_sha256=b.sha(app),app_crc32=zlib.crc32(app),descriptor_off=b.APP_OFF+64,ota_sha256=b.sha(ota))
    return bytes(app),ui,list(map(bytes,regions)),bytes(ota),binding


class BundleTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.app,cls.ui,cls.regions,cls.ota,cls.binding=synthetic()
        b.pm.set_board('smk37')
        old=b.pm.sdk_file
        b.pm.sdk_file=lambda name: bytes({'uboot.boot':14536,'cfg_tool.bin':383,'cfg/eq_cfg_hw.bin':655}[name])
        try:
            cls.template=b.pm.flash_image(cls.app,0x1234)
            cls.other_head=b.pm.flash_image(cls.app,0x5678)[:0x4000]
        finally:b.pm.sdk_file=old
        cls.raw=b.build_bundle(cls.template,cls.app,cls.ui,cls.regions,cls.ota,descriptor_off=cls.binding['descriptor_off'],loader_binding=cls.binding)

    def test_valid_and_per_device_conversion(self):
        before=b.validate_bundle(self.raw,loader_binding=self.binding)
        out,receipt=b.bind_head(self.raw,self.other_head,loader_binding=self.binding)
        after=b.validate_bundle(out,loader_binding=self.binding)
        self.assertEqual(before['canonical_main_sha256'],after['canonical_main_sha256'])
        self.assertEqual(b.unpack(out)[0][:0x4000],self.other_head)
        self.assertNotEqual(self.raw,out)
        self.assertEqual(receipt['source_sha256'],b.sha(self.raw))

    def test_binding_negatives(self):
        for key,value in [('app_sha256','0'*64),('ota_sha256','0'*64),('app_size',4095),('app_crc32',0),('descriptor_off',0x4000)]:
            bind=dict(self.binding);bind[key]=value
            with self.subTest(key=key),self.assertRaises(b.BundleError):b.validate_bundle(self.raw,loader_binding=bind)
        with self.assertRaises(b.BundleError):b.validate_bundle(self.raw,loader_binding={})

    def test_package_mutations(self):
        for at in (0,100,1044,40000,len(self.raw)-1):
            r=bytearray(self.raw);r[at]^=1
            with self.subTest(at=at),self.assertRaises((b.BundleError,b.add_ctl.PkgError)):
                b.validate_bundle(r,loader_binding=self.binding)
        product,lg=b.add_ctl.logical_of(self.raw)
        # Same character count, different model; no family wildcard admission.
        r=b.add_ctl.fwsc_of(lg,product.replace('Elite','ElitX'))
        with self.assertRaises(b.BundleError):b.validate_bundle(r,loader_binding=self.binding)
        for r in (self.raw[:-1],self.raw+b'\0'):
            with self.assertRaises(b.BundleError):b.validate_bundle(r,loader_binding=self.binding)

    def test_manifest_negative_with_valid_container(self):
        for i,at in ((0,0),(0,116),(1,0),(2,4095)):
            regions=list(self.regions);r=bytearray(regions[i]);r[at]^=1;regions[i]=bytes(r)
            with self.subTest(i=i,at=at),self.assertRaises(b.BundleError):
                b.build_bundle(self.template,self.app,self.ui,regions,self.ota,descriptor_off=self.binding['descriptor_off'],loader_binding=self.binding)
        for rs in (self.regions[:2],self.regions+[self.regions[2]]):
            with self.assertRaises(b.BundleError):b._manifest(rs)
        with self.assertRaises(b.BundleError):b.bind_head(self.raw,self.other_head[:-1],loader_binding=self.binding)
        h=bytearray(self.other_head);h[0x100]^=1
        with self.assertRaises(b.BundleError):b.bind_head(self.raw,h,loader_binding=self.binding)

    def test_immutable_response(self):
        self.assertEqual(b.response_bytes(b'abc',0,1,2,0xE0000000),(b'bc',False))
        self.assertEqual(b.response_bytes(b'abc',0,0xE0000000,8,0xE0000000),(b'success\0',True))
        for flags,addr,n,phase in ((1,0,1,0xE0000000),(0,0,0,0xE0000000),(0,0,513,0xE0000000),(0,2,2,0xE0000000),(0,0xFFFFFFFF,2,0xE0000000),(0,0xF0000000,8,0xE0000000),(0,0xE0000000,7,0xE0000000)):
            with self.subTest(addr=addr,n=n),self.assertRaises(b.BundleError):b.response_bytes(b'abc',flags,addr,n,phase)
        with self.assertRaises(b.BundleError):b.response_bytes(bytearray(b'abc'),0,0,1,0xE0000000)


def actual(root,sdk):
    root=Path(root);d=root/'components';m=json.loads((root/'components.json').read_text())
    b.pm.set_board('smk37');b.pm.SDK=Path(sdk)
    app=(d/'app.bin').read_bytes();ui=(d/'ui.bin').read_bytes();rs=[(d/f'vendor-{i}.bin').read_bytes() for i in range(3)];ota=(d/'ota.bin').read_bytes()
    for name,meta in m['artifacts'].items():
        data=(d/name).read_bytes()
        b.need(len(data)==meta['size'] and b.sha(data)==meta['sha256'],'component input identity')
    raw=b.build_bundle(b.pm.flash_image(app,0x1234),app,ui,rs,ota,descriptor_off=m['loader_binding']['descriptor_off'],loader_binding=m['loader_binding'])
    out,receipt=b.bind_head(raw,b.pm.flash_image(app,0x5678)[:0x4000],loader_binding=m['loader_binding'])
    print(json.dumps({'in_memory_only':True,'source_sha256':b.sha(raw),'converted_sha256':b.sha(out),'receipt':receipt},sort_keys=True))

if __name__=='__main__':
    if len(sys.argv)==3:actual(*sys.argv[1:])
    else:unittest.main()
