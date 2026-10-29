#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Independent float32 operation oracle from immutable original recipes.
No original musical recording and no numerical likeness threshold.
"""
import ctypes,json,struct,random,sys,hashlib
from pathlib import Path
f=lambda x:ctypes.c_float(x).value
bits=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
fl=lambda u:struct.unpack('<f',struct.pack('<I',u))[0]
lib=ctypes.CDLL(None);fma=lib.fmaf;fma.argtypes=[ctypes.c_float]*3;fma.restype=ctypes.c_float
mul=lambda a,b:f(a*b)
add=lambda a,b:f(a+b)
sub=lambda a,b:f(a-b)
def wave(shape,p,D,w):
 if shape==0:
  aa=sub(mul(f(D),2**-31),f(.024));b=mul(aa,8) if aa>0 else 0;k=fl(0xbeb020c5) if b>1 else mul(b,fl(0xbeb020c5))
  t=mul(2,sub(.5,abs(p)));return add(t,mul(k,mul(t,mul(t,t))))
 correction=1 if D<=0xffffff else mul(f((0x7fffffff000000//D)&0xffffffff),2**-31)
 def saw(x):return sub(x,max(-1,min(1,mul(mul(x,correction),128))))
 a=saw(p)
 if shape<2:return a
 q=add(p,add(w,w))
 if q>1:q=sub(q,2)
 return sub(a,saw(q))
for name,sha in [('Miami-MG-phase-waveform-PW-glide-and-A-sign-20261028.json','2186eb1676f72402bd36ee000196f6db89cea2cab7839e122426fe24d33956d2'),('Chicago-SYNC-XMOD-waveform-private-manifest-20261028.json','8f932e3f9dfc4e321c266c4a39e68ddcc1efa3792007a3536698c3d299a5bb7a')]:
 assert hashlib.sha256((Path(sys.argv[1])/name).read_bytes()).hexdigest()==sha
r=random.Random(798205)
out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
with (out/'wave.txt').open('w') as fh:
 n=0
 for D in [1,0xffffff,0x1000000,796253,42852281,1221665362,2147483647]:
  for shape in range(4):
   for p in [-1,-.5,-0.,0.,.5,1]+[f(r.uniform(-1,1)) for _ in range(256)]:
    for w in [0,.5,1,f(.099)]:
     fh.write(f'{shape} {bits(p):08x} {D} {bits(w):08x} {bits(wave(shape,p,D,w)):08x}\n');n+=1
print('PASS generated original separate-operation waveform fixtures',n)
# Read original Chicago arrays directly, independent of generated C header.
p=Path(sys.argv[1])/'Chicago-MP4-private-functional-generator-20261028.json';b=p.read_bytes();assert hashlib.sha256(b).hexdigest()=='7a75b90b3e288586e7d15b22d53ebd78f4a839cc8ca07b9c95858461866f3aaa'
d=json.loads(b)['generator']['tables_44100_derived_not_live_memory'];A=[fl(x) for x in d['A']['uint32_bits']];E=[fl(x) for x in d['E']['uint32_bits']]
def advance(cur,target,delta):
 q=add(cur,delta)
 return min(q,target) if delta>0 else max(q,target) if delta<0 else target
def feedback(x):
 u=0 if abs(x)>=1 else sub(1,abs(x));u2=mul(u,u);v=sub(1,mul(u,mul(u2,u2)));return v if x>=0 else -v
with (out/'mp4.txt').open('w') as fh:
 hist=[0.]*5;cur=[A[0],E[0],0.]
 for group in range(128):
  j=r.randrange(16385);R=f(r.random());D=r.choice([1,16,63,64,128]);kick=int(group%8==0)
  targets=[A[j],E[j],R];delta=[mul(sub(t,c),f(1/D)) for t,c in zip(targets,cur)]
  if kick:
   r2=mul(R,R);r4=mul(r2,r2);hist[0]=mul(r4,r4) if R>f(.2) else 0.
  fh.write(f'P {j} {bits(R):08x} {D} {kick}\n')
  for frame in range(D):
   cur=[advance(c,t,d) for c,t,d in zip(cur,targets,delta)];a,e,R=cur;x=f(r.uniform(-.5,.5))
   tmp=add(hist[3],mul(e,hist[4]));v=sub(x,mul(R,feedback(sub(hist[4],mul(e,tmp)))))
   z1=add(v,mul(a,sub(hist[0],v)));z2=add(z1,mul(a,sub(hist[1],z1)));z3=add(z2,mul(a,sub(hist[2],z2)));z4=add(z3,mul(a,sub(hist[3],z3)));hist=[z1,z2,z3,z4,tmp]
   fh.write('S '+' '.join(f'{bits(v):08x}' for v in [x,*hist])+'\n')
print('PASS generated Chicago exact state trajectories with original derived tables, kicks and ramps')
# Original phase subtraction/XMod/sync oracle, uint32 modulo and explicit saturation.
def signed(u):return u if u<0x80000000 else u-0x100000000
with (out/'phase.txt').open('w') as fh:
 for sync in range(2):
  for run in range(128):
   P=[r.getrandbits(32) for _ in range(4)];D=[r.randrange(1,1221665363) for _ in range(4)];shapes=[r.randrange(4) for _ in range(4)];widths=[f(r.random()) for _ in range(4)];gains=[f(r.random()) for _ in range(4)];C=f(r.uniform(0,f(.9)))
   fh.write('I '+' '.join(map(str,[sync,*P,*D,*shapes]))+' '+' '.join(f'{bits(v):08x}' for v in [*widths,*gains,C])+'\n')
   for frame in range(32):
    waves=[0.]*4
    for i in [0,2]:
     old=P[i];P[i]=(old-D[i])&0xffffffff;waves[i]=wave(shapes[i],mul(f(signed(P[i])),2**-31),D[i],widths[i])
     step=mul(max(-1,min(1,add(mul(waves[i],C),mul(f(D[i+1]),2**-31)))),2**31)
     integer=max(-2147483648,min(2147483647,int(step)));P[i+1]=(P[i+1]-integer)&0xffffffff
     if sync and signed(P[i])>signed(old):P[i+1]=0x7fffffff
     waves[i+1]=wave(shapes[i+1],mul(f(signed(P[i+1])),2**-31),D[i+1],widths[i+1])
    mix=mul(waves[0],mul(gains[0],.5))
    for i in range(1,4):mix=add(mix,mul(waves[i],mul(gains[i],.5)))
    fh.write('S '+' '.join(map(str,P))+f' {bits(mix):08x}\n')
print('PASS generated 8192 exact four-oscillator phase/mix frames across Miami and Chicago')
with (out/'glide.txt').open('w') as fh:
 for run in range(256):
  current=[f(r.uniform(0,120)) for _ in range(4)];target=[f(r.uniform(0,120)) for _ in range(4)];port=f(r.random());mod=f(r.uniform(-1,1));fs=r.choice([8000,44100,48000,192000]);D=int(__import__('math').ceil(fs/690))
  fh.write('I '+' '.join(f'{bits(v):08x}' for v in [*current,*target,port,mod])+f' {fs} {D}\n')
  t=max(0,min(1,add(port,mod)));a=fma(mul(t,t),fl(0x49f42400),fl(0x3a83126f));a=mul(fs,a);a=f(D/a);a=mul(a,fl(0x4a8ab75d));alpha=min(a,1)
  for tick in range(32):
   current=[fma(sub(t,c),alpha,c) for t,c in zip(target,current)]
   fh.write('S '+' '.join(f'{bits(v):08x}' for v in current)+'\n')
print('PASS generated 8192 exact four-pitch fused glide ticks')
