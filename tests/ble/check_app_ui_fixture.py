#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import importlib.util
from pathlib import Path
import sys
import struct
root=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location('fixture',root/'tools/studies/smc_app_data/seal_fixture.py')
m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
elf=Path(sys.argv[1]).read_bytes();app=Path(sys.argv[2]).read_bytes()
patched,ui,meta=m.seal(elf,app)
def bad(e,a):
    try:m.seal(e,a)
    except (ValueError,UnicodeError):return
    raise AssertionError('mismatched/untrusted source accepted')
for i in (0,100,len(app)//2,len(app)-1):
    a=bytearray(app);a[i]^=1;bad(elf,a)
bad(elf,app[:-4]);bad(elf,app+b'1234');bad(elf,patched)
for n in (0,40,51,len(elf)-1):bad(elf[:n],app)
changes=[i for i,(a,b) in enumerate(zip(app,patched)) if a!=b]
assert changes and all(meta['descriptor_image_offset']<=i<meta['descriptor_image_offset']+20 for i in changes)
print('app UI sealer: exact ELF/source binding, changed/truncated/resealed rejection PASS')

# The external section is not in app.bin: ensure accidental SPL record tags are
# rejected even when every original primary app byte still matches its ELF.
shoff=struct.unpack_from('<I',elf,32)[0]
ssz,snum=struct.unpack_from('<HH',elf,46)
for i in range(snum):
    row=struct.unpack_from('<10I',elf,shoff+i*ssz)
    if row[3]==0x020A7000:
        e=bytearray(elf);struct.pack_into('<H',e,row[4]+0x1F06,0x5441);bad(e,app)
        break
else:raise AssertionError('missing external UI section')
print('UI spare-sector update-record tag rejection PASS')
