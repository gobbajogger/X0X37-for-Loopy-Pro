#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Independent golden bytes and rejection contract for website converters."""
import importlib.util, pathlib, random
root=pathlib.Path(__file__).resolve().parents[1]
s=importlib.util.spec_from_file_location('boot_media',root/'tools/boot_media.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
checks=0
def check(v):
 global checks
 checks+=1;assert v
def reject(f):
 global checks
 try:f()
 except ValueError:checks+=1;return
 raise AssertionError('accepted invalid input')
solid=(root/'tests/fixtures/boot_media/solid.bm').read_bytes()
check(m.pack([0],[bytes(3200)],[1000])==solid)
x=m.unpack(solid);check(x['frames']==[bytes(3200)] and x['durations']==[1000])
fixed=(root/'tests/fixtures/boot_media/dnbox.bm').read_bytes();x=m.unpack(fixed,m.COMPILED_MAX)
check(len(x['frames'])==27 and sum(x['durations'])==2743)
reject(lambda:m.unpack(fixed));reject(lambda:m.pack([0],[bytes(3200)],[6000]))
reject(lambda:m.pack([0],[bytes([0x11])*3200],[100]))
reject(lambda:m.pack([0],[bytes(3199)],[100]))
reject(lambda:m.pack([0],[bytes(3200)],[100],100))
for n in range(len(solid)):reject(lambda n=n:m.unpack(solid[:n]))
for i in (0,1,2,4,6,8,10,12,16,20,24,28,34,64,66):
 bad=bytearray(solid);bad[i]^=1;reject(lambda bad=bad:m.unpack(bad))
r=random.Random(47)
for n in (1,2,5):
 frames=[bytes(r.randrange(16)*17 for _ in range(3200)) for _ in range(n)]
 packed=m.pack(list(range(16)),frames,[100]*n,m.COMPILED_MAX)
 check(m.unpack(packed,m.COMPILED_MAX)['frames']==frames)
print(f'PASS {checks} converter checks')
