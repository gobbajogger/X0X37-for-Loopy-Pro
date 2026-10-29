#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exact rational binary32 oracle; no host libm, original code, or DSP rendering.
Generates private local fixtures and checks supplied ARM arithmetic witnesses.
"""
import argparse, hashlib, json, random
from fractions import Fraction as F
PIN='146cd9684f1ebbdbd0c2d5ae2e67b01b0af434a0a6fc104da6dba5b2948f18dd'
def value(u):
    sign=-1 if u>>31 else 1; e=(u>>23)&255; m=u&0x7fffff
    assert e!=255
    return sign*F(m if e==0 else m+0x800000)*F(2)**(e-150 if e else -149)
def rounded(x):
    if not x:return 0
    sign=0x80000000 if x<0 else 0; x=abs(x)
    e=x.numerator.bit_length()-x.denominator.bit_length()
    if x<F(2)**e:e-=1
    step=max(e-23,-149); y=x/F(2)**step
    q,r=divmod(y.numerator,y.denominator)
    if 2*r>y.denominator or (2*r==y.denominator and q&1):q+=1
    if q==0:return sign
    if q>=0x1000000:q>>=1;step+=1
    if step==-149 and q<0x800000:return sign|q
    e=step+23
    if e>127:return sign|0x7f800000
    return sign|((e+127)<<23)|(q-0x800000)
def cleanup(u):
    m=u&0x7fffffff
    if m>0x7f800000 or m<=0x3f000000:return u
    z=F(1) if m>=0x3f800000 else abs(value(u))
    a=value(rounded(2*z-1)); b=value(rounded(1-a)); c=value(rounded(1-b*b))
    result=rounded(c/4+F(1,2))
    return result|(u&0x80000000)
def scaled(u,fz):
    m=u&0x7fffffff
    if m>0x7f800000:return u|0x400000
    if m==0x7f800000:return u
    if m==0 or (fz and m<0x800000):return u&0x80000000
    r=rounded(value(u)*32767)
    if fz and (r&0x7fffffff)<0x800000:r&=0x80000000
    return r
def cvt(u):
    m=u&0x7fffffff
    if m>0x7f800000:return 0
    if m==0x7f800000:return -(1<<31) if u>>31 else (1<<31)-1
    return min((1<<31)-1,max(-(1<<31),int(value(u))))
def narrow(n):return (n+32768)%65536-32768
def back(n):return rounded(F(n,32767))
def chain(u,fz):
    c=cleanup(u);s=scaled(c,fz);i=cvt(s);n=narrow(i)
    return c,s,i,n,back(n)
def main():
    p=argparse.ArgumentParser();p.add_argument('manifest');p.add_argument('output');a=p.parse_args()
    data=open(a.manifest,'rb').read();assert hashlib.sha256(data).hexdigest()==PIN
    sets=json.loads(data)['witness_provenance']['witnesses']['sets']; witness_count=0
    for sample in sets:
        fz=int(sample['fpcr'],16)>>24&1
        for row in sample['cleanup_chain']:
            got=chain(int(row['input'],16),fz)
            want=tuple(int(row[k],16) if k in ('clean','scaled','return_float') else row[k] for k in ('clean','scaled','integer','low16_signed','return_float'))
            assert got==want,(row,got,want);witness_count+=1
        for row in sample['integer_thresholds']:
            assert scaled(cleanup(int(row['input'],16)),fz)==int(row['scaled'],16)
            assert cvt(int(row['scaled'],16))==row['integer'];witness_count+=1
        for row in sample['direct_fcvtzs']:
            assert cvt(int(row['input'],16))==row['integer'];witness_count+=1
        for row in sample['direct_narrow']:
            assert narrow(row['integer'])==row['low16_signed'];witness_count+=1
        for row in sample['direct_return']:
            assert back(row['integer'])==int(row['fdiv_return'],16);witness_count+=1
    inputs={int(r['input'],16) for s in sets for r in s['cleanup_chain']+s['integer_thresholds']+s['direct_fcvtzs']}
    rng=random.Random(62532)
    inputs.update(rng.getrandbits(32) for _ in range(8192))
    # Both sides of every reachable integer threshold, including softclip region.
    for n in range(-24576,24577):
        u=rounded(F(n,32767))
        inputs.update((max(0,u-1),u,min(0xffffffff,u+1)))
    with open(a.output,'w') as out:
        for u in sorted(inputs):
            for fz in (0,1):
                c,s,i,n,b=chain(u,fz)
                out.write(f'C {fz} {u:08x} {c:08x} {s:08x} {i} {n} {b:08x}\n')
            out.write(f'D {u:08x} {cvt(u)}\n')
        for n in range(-32768,32768):out.write(f'O {n} {back(n):08x}\n')
    print(f'PASS rational oracle agrees with {witness_count} ARM witness rows; {len(inputs)*2} chain cases, {len(inputs)} direct FCVTZS cases, 65536 FDIV outputs')
if __name__=='__main__':main()
