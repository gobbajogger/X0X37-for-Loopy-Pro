#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Private functional tables, traced recipes; derived libm, not live-memory proof."""
import argparse,ctypes,hashlib,json,math,struct
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('reference_directory');p.add_argument('ms20_manifest');p.add_argument('output');args=p.parse_args()
base=Path(args.reference_directory)
def read(name,sha):
 b=(base/name).read_bytes();assert hashlib.sha256(b).hexdigest()==sha;return json.loads(b)
cor=read('Miami-MS20-cap-runtime-correction-and-Chicago-ARP-20261028.json','e10e582845c017917243b6312a459375ca9873ae64b445f9a164a11239ef0815')
mg=read('Miami-MG-phase-waveform-PW-glide-and-A-sign-20261028.json','2186eb1676f72402bd36ee000196f6db89cea2cab7839e122426fe24d33956d2')
ch=read('Chicago-MP4-private-functional-generator-20261028.json','7a75b90b3e288586e7d15b22d53ebd78f4a839cc8ca07b9c95858461866f3aaa')
b=Path(args.ms20_manifest).read_bytes();assert hashlib.sha256(b).hexdigest()=='26bffc368d7bedba4bb5cb8df17dda9e2e18e13c2216a4affc5f5987704eef98'
f=lambda x:ctypes.c_float(x).value
fl=lambda u:struct.unpack('<f',struct.pack('<I',u))[0]
db=lambda u:struct.unpack('<d',struct.pack('<Q',u))[0]
lib=ctypes.CDLL(None);fma=lib.fma;fma.argtypes=[ctypes.c_double]*3;fma.restype=ctypes.c_double
cap=cor['cutoff_cap129']['uint32_bits'];assert len(cap)==129
assert hashlib.sha256(struct.pack('<129I',*cap)).hexdigest()=='30877e172ddc7d9f3a92e902178322d38fbccae746cbb94c9fe8a280146c0b14'
assert cap[108]==0x3f820834 and cap[109]==0x3f8020c5
A=[];B=[];R=[]
for i in range(129):
 freq=min((523.2511306011972*math.exp2((i-64)/12) if i<128 else 21096.16364242367)/44100,.5)
 s=math.sin(2*math.pi*freq);c=math.cos(2*math.pi*freq);s2=math.sin(4*math.pi*freq);c2=math.cos(4*math.pi*freq)
 k=fma(-2,c,4);A.append(f((math.sqrt(fma(k,k,-4))-k)*-.5))
 t=math.tan(fma(freq,-math.pi,math.pi/2));a=s2-s;a=fma(-t,c,a);a=fma(t,c2,a);a=a-s2
 bb=(t+t)*c;bb=fma(s,2,bb);d=s-s2;d=fma(t,c,d);d=fma(-t,c2,d);d=d-t;a=fma(-t,c2,a)
 B.append(f((math.sqrt(fma(bb,bb,d*(-4*a)))-bb)/(a+a)))
 if i==128:R.append(fl(0x3f949bad))
 else:
  u=i*(1/128);v=1/(1-u);v=v+db(0x40122e8ba2e8ba2f);v=1/v;v=u+v;v=u/v
  R.append(max(f(f(v)*fl(0x3f949bad)),fl(0x3727c5ac)))
assert all(math.isfinite(v) for arr in [A,B,R] for v in arr)
for i,u in [(0,0x3f7f86ee),(64,0x3f6d9e06),(128,0x3e3043e8)]:assert f(A[i])==fl(u)
out='/* PRIVATE functional coefficients. Derived 44100-Hz libm tables; not live original memory. */\n#include <stdint.h>\n'
arrays={'original_miami_a44100':A,'original_miami_b44100':B,'original_miami_r':R,'original_miami_cap':[fl(u) for u in cap]}
for key in ['A','E']:
 tab=ch['generator']['tables_44100_derived_not_live_memory'][key];u=tab['uint32_bits'];assert len(u)==16385
 assert hashlib.sha256(struct.pack('<16385I',*u)).hexdigest()==tab['sha256_little_endian_float32'];arrays['original_chicago_'+key.lower()+'44100']=[fl(x) for x in u]
for name,arr in arrays.items():
 out+=f'static const float {name}[{len(arr)}]={{\n'+'\n'.join(','.join(v.hex()+'f' for v in arr[i:i+8])+',' for i in range(0,len(arr),8))+'\n};\n'
values=mg['portamento_and_pitch']['integer_phase_table_values'];assert len(values)==128
assert hashlib.sha256(struct.pack('<128i',*values)).hexdigest()=='77c39cdfb56f1cd4da2bfbe83d1c7902a0a8078e0b66f98045a49d0e9dc20926'
out+='static const int32_t original_pitch44100[128]={'+','.join(map(str,values))+'};\n'
Path(args.output).write_text(out)
print('PASS private corrected cap, MS20 derived A/B/R anchors, Chicago transferred derived A/E and exact pitch table; no live-memory or acoustic claim')
