#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Lossless private table encoding, exact binary32 data; not reduced precision."""
import json,hashlib,struct,sys
from pathlib import Path
b=Path(sys.argv[1]).read_bytes();assert hashlib.sha256(b).hexdigest()=='7a75b90b3e288586e7d15b22d53ebd78f4a839cc8ca07b9c95858461866f3aaa'
d=json.loads(b)['generator']['tables_44100_derived_not_live_memory'];s='/* PRIVATE exact original-derived table bits, losslessly encoded. */\n#include "lossless_table.h"\n';report={}
for name in ['A','E']:
 t=d[name];v=t['uint32_bits'];assert len(v)==16385;assert hashlib.sha256(struct.pack('<16385I',*v)).hexdigest()==t['sha256_little_endian_float32'];data=bytearray();offset=[]
 for i in range(0,len(v),32):
  x=v[i:i+32];offset.append(len(data));z=[]
  for a,b in zip(x,x[1:]):
   delta=(b-a)&0xffffffff;delta=delta if delta<2**31 else delta-2**32;z.append(((delta<<1)^(delta>>31))&0xffffffff)
  w=max(z,default=0).bit_length();data.extend(struct.pack('<IB',x[0],w));packed=sum(n<<(k*w) for k,n in enumerate(z));data.extend(packed.to_bytes((len(z)*w+7)//8,'little'))
 symbol='original_chicago_packed_'+name.lower()
 s+=f'static const uint8_t {symbol}_data[{len(data)}]={{\n'+'\n'.join(','.join(map(str,data[i:i+24]))+',' for i in range(0,len(data),24))+'\n};\n'
 s+=f'static const uint32_t {symbol}_offset[{len(offset)}]={{'+','.join(map(str,offset))+'};\n'
 s+=f'static const original_lossless_table {symbol}={{{symbol}_data,{symbol}_offset,sizeof {symbol}_data,16385,{len(offset)}}};\n'
 report[name]={'payload':len(data),'directory':4*len(offset),'total':len(data)+4*len(offset),'source_sha256':t['sha256_little_endian_float32'],'payload_sha256':hashlib.sha256(data).hexdigest()}
Path(sys.argv[2]).write_text(s);print(json.dumps({'tables':report,'total':sum(v['total'] for v in report.values()),'raw':131080,'precision':'All 32770 binary32 values exact; no interpolation or approximation'},indent=2))
