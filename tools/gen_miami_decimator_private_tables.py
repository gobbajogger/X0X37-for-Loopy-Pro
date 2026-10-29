#!/usr/bin/env python3
"""Private functional coefficients only; pinned original handoff, no extrapolation."""
import hashlib,json,pathlib,struct,sys,tempfile,os
PIN='e5cbbe1541fb8f963d6935c9dfb599dfe60e32d63a25bc10b310db5c9ddda490'
def generate(source,out):
 raw=source.read_bytes()
 if hashlib.sha256(raw).hexdigest()!=PIN:raise ValueError('manifest pin')
 tables=json.loads(raw)['functional_tables'];lines=['/* PRIVATE ORIGINAL FUNCTIONAL COEFFICIENTS: no public delivery. */','#ifndef MIAMI_DECIMATOR_TABLES_PRIVATE_H','#define MIAMI_DECIMATOR_TABLES_PRIVATE_H','#include <stdint.h>',f'/* manifest SHA256 {PIN} */']
 for name,fmt,ctype,maxv in [('frequency','H','uint16_t',65535),('wet','h','int16_t',32767)]:
  t=tables[name];v=t['values']
  if len(v)!=128 or any(isinstance(x,bool) or not isinstance(x,int) or not 0<=x<=maxv for x in v):raise ValueError('coefficient domain')
  if hashlib.sha256(struct.pack('<128'+fmt,*v)).hexdigest()!=t['source_raw_sha256']:raise ValueError('table hash')
  lines.extend([f'/* original raw table SHA256 {t["source_raw_sha256"]} */',f'static const {ctype} miami_decimator_{name}[128] = {{'])
  lines.extend(' '+','.join(map(str,v[i:i+16]))+',' for i in range(0,128,16));lines.append('};')
 lines.append('#endif');out.parent.mkdir(parents=True,exist_ok=True);fd,tmp=tempfile.mkstemp(dir=out.parent)
 try:
  with os.fdopen(fd,'w') as f:f.write('\n'.join(lines)+'\n')
  os.replace(tmp,out)
 finally:
  if os.path.exists(tmp):os.unlink(tmp)
if __name__=='__main__':generate(pathlib.Path(sys.argv[1]),pathlib.Path(sys.argv[2]))
