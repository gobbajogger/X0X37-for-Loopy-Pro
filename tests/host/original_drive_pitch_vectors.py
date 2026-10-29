#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import json,hashlib,struct,ctypes,random,sys
from pathlib import Path
root=Path(sys.argv[1]);out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
b=(root/'final-current-turn/Chicago-Bite-Drive-private-functional-manifest-20261028.json').read_bytes();assert hashlib.sha256(b).hexdigest()=='bb0fcb9314a99135ea0c662af5dedd476021e9747a2489d8ce821b407393e9f6';m=json.loads(b)
D=m['functional_tables']['Drive']['values_int16'];G=m['functional_tables']['Gain']['values_int16']
def I(v):return ((v+2**31)%2**32)-2**31
def low(v):return ((v+2**15)%2**16)-2**15
class Drive:
 def __init__(self):self.td=16254;self.tg=20801;self.sd=self.sg=self.st=0;self.pre=[0,0];self.y=[0,0];self.active=0
 def reset(self):self.sd=I(self.td<<15);self.sg=I(self.tg<<15);self.st=0;self.pre=[0,0];self.y=[0,0]
 def frame(self,l,r):
  k=32767 if self.active else 102
  def sm(cur,t):return I(I(cur-I((cur>>15)*k))+I(k*t))
  self.sd=sm(self.sd,self.td);self.sg=sm(self.sg,self.tg);self.st=sm(self.st,max(19660-self.td,0));d=self.sd>>15;g=self.sg>>15;t=self.st>>15;fb=I(d*23919)>>15;outputs=[]
  for ch,x in enumerate([l,r]):
   y=self.y[ch];a=I(I(y<<1)-I(y<<15));bb=I((a>>15)<<1);F=I(bb-I(a&0xffff8000))>>15
   v=I(I(self.pre[ch]+x)+(I(F*fb)>>15));driven=I(d*x)>>13;v=max(-32767,min(32767,v));z=max(abs(v)-t,0);z2=(I(z*z)&0xffffffff)>>15;sign=26213 if v>0 else -26213 if v else 0;y=I(v-(I(z2*sign)>>15));self.pre[ch]=driven;self.y[ch]=y;outputs.append(low((I(y*g)&0xffffffff)>>15))
  return outputs
# Reproduce manifest's separate derived integer witnesses before generating tests.
s=Drive();s.td=7482;s.tg=24900;s.reset()
for w in m['own_integer_witnesses']['frames']:
 assert s.frame(*w['input'])==w['output'];v=w['state'];assert [s.sd,s.sg,s.st,*s.pre,*s.y]==[v[k] for k in ['SD','SG','ST','preL','preR','yL','yR']]
r=random.Random(625) # Fixed independent integer oracle seed.
with (out/'drive.txt').open('w') as fh:
 s=Drive();fh.write('I\n');count=0
 def emit(l,r):
  global count
  result=s.frame(l,r);fh.write('S '+' '.join(map(str,[l,r,*result,s.sd,s.sg,s.st,*s.pre,*s.y]))+'\n');count+=1
 for activity in [0,1,255]:
  s.active=activity;fh.write(f'A {activity}\n')
  for d in range(128):
   s.td=D[d];fh.write(f'P 2 {d}\n')
   for g in range(128):
    s.tg=1279+(I(G[g]*31489)>>15);fh.write(f'P 3 {g}\n')
    if g%31==0:s.reset();fh.write('R\n')
    emit(r.choice([-32768,-32767,-1,0,1,32767]),r.randrange(-32768,32768))
 for _ in range(32768):
  if r.randrange(80)==0:s.reset();fh.write('R\n')
  if r.randrange(32)==0:
   s.active=r.choice([0,1,255]);fh.write(f'A {s.active}\n')
  emit(r.randrange(-32768,32768),r.randrange(-32768,32768))
print('PASS manifest witnesses and',count,'integer Drive oracle frames; all 128x128 targets, three activity states')
# Four-pitch selected Miami binding; original manifest checksum is a prerequisite.
b=(root/'final-current-turn/Miami-constructor-scale-four-pitch-closure-20261028.json').read_bytes();assert hashlib.sha256(b).hexdigest()=='5447f5818e6b510cc9c0296bf808029eceb5fcbc9220f9a9ae4492d3f34e5184'
p=json.loads((root/'oscillator-filter-closure/Miami-MG-phase-waveform-PW-glide-and-A-sign-20261028.json').read_text())['portamento_and_pitch']['integer_phase_table_values']
f=lambda x:ctypes.c_float(x).value
bits=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
lib=ctypes.CDLL(None);fma=lib.fmaf;fma.argtypes=[ctypes.c_float]*3;fma.restype=ctypes.c_float
with (out/'pitch.txt').open('w') as fh:
 for n in range(8192):
  current=[f(r.uniform(0,127)) for _ in range(4)];target=r.randrange(128);coordinate=r.choice([-.5,.5]);bend=f(r.uniform(-1,1));slave=f(r.uniform(-.5,.5));port=f(r.uniform(0,.6036866307));semi1=r.randrange(-24,25);semi3=r.randrange(-24,25);duration=r.choice([63,64]);fine=f(.003000020980834961)
  inputs=[*current,float(target),coordinate,bend,slave,port];fh.write(' '.join(f'{bits(x):08x}' for x in inputs)+f' {semi1} {semi3} {duration} ')
  a=f(port*port);a=fma(a,2000000,f(.001));a=f(44100*a);a=f(duration/a);a=f(a*f(4545454.5));a=min(a,1);current=[fma(f(target-c),a,c) for c in current]
  common=f(0.+0.);common=fma(coordinate,f(.017999999225139618),common);common=fma(0.,48,common);common=f(common+0.);common=fma(12,bend,common)
  increments=[]
  for i in range(4):
   x=f(common+current[i]);x=f(x+[semi1,-12,semi3,0][i]);x=fma(0.,48,x)
   if i:x=f([0.,0.,fine,-fine][i]+x)
   if i==1:x=f(x-0.)
   if i==3:x=f(0.+x)
   if i in [1,3]:x=f(0.+x);x=fma(slave,48,x)
   x=max(0,min(120,x));j=int(x);frac=f(x-f(j));increments.append(p[j]+int(f(f(p[j+1]-p[j])*frac)))
  fh.write(' '.join(f'{bits(x):08x}' for x in current)+' '+' '.join(map(str,increments))+'\n')
print('PASS 8192 selected-context four-pitch oracle cases including full MIDI note range')
