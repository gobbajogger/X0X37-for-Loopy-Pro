#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Compare unmodified functions and semantic ELF32 relocation targets.
import struct,json,hashlib,sys
from pathlib import Path

def read(path):
 b=Path(path).read_bytes(); assert b[:6]==b'\x7fELF\x01\x01'
 e=struct.unpack_from('<16sHHIIIIIHHHHHH',b); off,esz,n,ss=e[6],e[11],e[12],e[13]
 secs=[struct.unpack_from('<10I',b,off+i*esz) for i in range(n)]
 def data(i): s=secs[i]; return b[s[4]:s[4]+s[5]]
 def string(tab,i): return tab[i:].split(b'\0',1)[0].decode()
 names=[string(data(ss),s[0]) for s in secs]; result={}
 for i,name in enumerate(names):
  if not name.startswith('.text.'):continue
  rel=[]
  for j,s in enumerate(secs):
   if s[1]!=4 or s[7]!=i:continue
   symsec=secs[s[6]]; syms=data(s[6]); strings=data(symsec[6])
   for k in range(0,s[5],s[9]):
    ro,ri,ra=struct.unpack_from('<IIi',data(j),k)
    sn,sv,sz,si,so,sh=struct.unpack_from('<IIIBBH',syms,(ri>>8)*symsec[9])
    sym=string(strings,sn) or (names[sh] if sh<len(names) else str(sh))
    rel.append((ro,ri&255,sym,sv,ra))
  result[name]=(data(i),rel)
 return result
if len(sys.argv) != 3:
 raise SystemExit('usage: compare_drum909_init_target.py BASELINE.o CANDIDATE.o')
base=read(sys.argv[1])
cand=read(sys.argv[2])
out={}
for k in base:
 if k=='.text.drum909_init':continue
 assert base[k]==cand[k], k
 out[k]={'bytes':len(base[k][0]),'sha256':hashlib.sha256(base[k][0]).hexdigest(),'relocations':len(base[k][1]),'identical_bytes_and_symbolic_relocations':True}
print(json.dumps(out,indent=2))
print('PASS',len(out),'unchanged function sections including symbolic relocation targets')
