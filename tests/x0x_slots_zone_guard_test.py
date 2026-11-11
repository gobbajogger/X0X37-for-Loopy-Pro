#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""No-hardware malformed archive guards (the real repository imports tools/x0x_slots.py)."""
import json
import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import x0x_slots as xs


def archive(patch=None):
    data=bytes(range(64)); hdr=bytearray(b'\xff'*xs.DATA_OFF)
    struct.pack_into('<IHBB8sII',hdr,0,xs.MAGIC,1,1,0,b'GOOD\x00\x00\x00\x00',len(data),zlib.crc32(data))
    struct.pack_into('<5I',hdr,32,0,len(data)*2,0,len(data)*2,1<<16)
    struct.pack_into('<h',hdr,52,60*16);hdr[56]=0
    if patch: patch(hdr)
    img=bytes(hdr)+data
    meta={'format':'x0xslots','version':1,'created':'2026-10-23T00:00:00Z','count':1,'size':0x14000,
          'slots':[{'slot':1,'size':0x14000,'empty':False,'offset':0,'length':len(img),'crc32':zlib.crc32(img),
                    'name':'GOOD','zones':1,'rates':[44100],'samples':[128],'data_len':len(data),'data_crc32':zlib.crc32(data)}]}
    js=json.dumps(meta,separators=(',',':'),ensure_ascii=True).encode('ascii')
    body=xs.ARCH_MAGIC+struct.pack('<HHI',1,0,len(js))+js+img
    return body+struct.pack('<I',zlib.crc32(body))


def invalid_offset(b): struct.pack_into('<I',b,32,65)
def invalid_count(b): struct.pack_into('<I',b,36,129)
def invalid_rate0(b): struct.pack_into('<I',b,48,0)
def invalid_rate_hi(b): struct.pack_into('<I',b,48,(4<<16)+1)
def invalid_adpcm_idx(b): b[56]=89


class FlashMock:
    """Emulates first-write erase and firmware END header refusal; no actual USB/flash."""
    def __init__(self): self.existing=b'old-user-samples';self.current=self.existing;self.commands=[]
    def req(self,cmd,a,timeout):
        self.commands.append(cmd)
        if cmd==xs.CMD_BEGIN: self.current=b'';return [0,0]
        if cmd==xs.CMD_WRITE:return [0,0,0,0,0]
        if cmd==xs.CMD_END:return [0,5]
        raise RuntimeError('unexpected cmd')

n=0; fails=0
def check(b,descr):
    global n,fails
    n+=1
    print(('PASS' if b else 'FAIL'),descr)
    fails+=not b

m,ims=xs.read_archive(archive())
check(len(ims)==1 and len(ims[0])==576,'canonical header accepted')
for name,mut in [('zone offset beyond data',invalid_offset),('ADPCM n exceeds data',invalid_count),
                 ('zero Q16 rate',invalid_rate0),('excessive Q16 rate',invalid_rate_hi),
                 ('invalid ADPCM predictor index',invalid_adpcm_idx)]:
    try:
        xs.read_archive(archive(mut))
    except xs.SlotError:
        check(True,'reject '+name)
    else:
        check(False,'reject '+name)

# A direct mock of the host restoration path confirms the erase-before-END ordering.
# This is an intentionally *observed* risk simulation, not a physical-device result.
mock=FlashMock()
try:
    meta,images=xs.read_archive(archive(invalid_adpcm_idx))
    xs.write_slot(mock,0,images[0])
except xs.SlotError:
    pass
check(mock.current==mock.existing and not any(c in mock.commands for c in (xs.CMD_BEGIN,xs.CMD_ERASE)),
      'bad archive refused before any destructive mock command')

# damaged CRC remains rejected
p=bytearray(archive());p[-1]^=1
try:xs.read_archive(bytes(p))
except xs.SlotError:check(True,'outer CRC still enforced')
else:check(False,'outer CRC still enforced')
print(f'CHECKS={n} FAILURES={fails}')
sys.exit(1 if fails else 0)
