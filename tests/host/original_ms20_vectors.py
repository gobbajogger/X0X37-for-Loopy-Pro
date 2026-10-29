#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import importlib.util,json,struct,hashlib,random,sys
from pathlib import Path
spec=importlib.util.spec_from_file_location('oracle',Path(__file__).with_name('miami_fx_conversion_vectors.py'));o=importlib.util.module_from_spec(spec);spec.loader.exec_module(o)
b=Path(sys.argv[1]).read_bytes();assert hashlib.sha256(b).hexdigest()=='26bffc368d7bedba4bb5cb8df17dda9e2e18e13c2216a4affc5f5987704eef98'
x=json.loads(b);table=x['nonlinearity_N513']['uint32_bits'];assert hashlib.sha256(struct.pack('<513I',*table)).hexdigest()=='4880416fdae25c3fef0c3885c358f65a5f42d4470f0f6e7d8d8922a115de800c'
def nonlinear(u):
 sign=u&0x80000000;mag=u&0x7fffffff
 if mag==0:return u
 v=o.value(u)
 if mag>=0x3f800000:return table[512]|sign
 q=o.rounded(abs(v)*512);qv=o.value(q);j=int(qv);f=o.value(o.rounded(qv-j));diff=o.value(o.rounded(o.value(table[j+1])-o.value(table[j])))
 gain=o.value(o.rounded(diff*f+o.value(table[j])))
 return o.rounded(v*gain)
for w in x['nonlinearity_N513']['witnesses']:assert nonlinear(int(w['input_bits'],16))==int(w['output_bits'],16)
inputs={0,0x80000000,0x7f7fffff,0xff7fffff,1,0x80000001}
for j in range(513):
 u=o.rounded(o.F(j,512))
 for q in (max(0,u-1),u,u+1):inputs.update((q,q|0x80000000))
r=random.Random(513)
for _ in range(8192):
 u=r.getrandbits(32)
 if (u&0x7f800000)!=0x7f800000:inputs.add(u)
with open(sys.argv[2],'w') as f:
 for u in sorted(inputs):f.write(f'{u:08x} {nonlinear(u):08x}\n')
print('PASS N513 exact table/hash/witness rational fixtures',len(inputs))
