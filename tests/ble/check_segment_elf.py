#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Exercise the extractor using a private pinned target ELF; never writes flash."""
import importlib.util
from pathlib import Path
import struct
import sys
root=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('seal',root/'tools/studies/smc_reference/seal_segments.py')
m=importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
b=Path(sys.argv[1]).read_bytes()
images,meta=m.extract(b,'SMK-37 Elite')
assert sum(map(len,images))>0 and meta['ram_hi']<=0x01c07f00

def reject(x):
    try: m.extract(x,'SMK-37 Elite')
    except (ValueError,UnicodeError): return
    raise AssertionError('malformed ELF accepted')
for n in (0,6,16,40,51,len(b)-1): reject(b[:n])
for off,fmt,value in ((4,'B',2),(5,'B',2),(16,'H',1),(18,'H',0),
                       (28,'I',0xfffffff0),(32,'I',0xfffffff0),(42,'H',1),(46,'H',1)):
    x=bytearray(b);struct.pack_into('<'+fmt,x,off,value);reject(x)
shoff=struct.unpack_from('<I',b,32)[0]
size,count=struct.unpack_from('<HH',b,46)
# Every allocated section's VMA must remain inside its named admitted window.
for i in range(count):
    at=shoff+i*size
    row=struct.unpack_from('<10I',b,at)
    if row[2]&2 and row[5]:
        x=bytearray(b);struct.pack_into('<I',x,at+12,0x01c7fd50);reject(x)
print('ELF fixture extractor: malformed tables, truncation, type and protected VMA rejection PASS')
