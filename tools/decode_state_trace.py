#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Decode exact-ELF mode-1 input timing trace RAM (layout v4)."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
FIELDS=('half','late','key18_tick','key22_tick','consume_tick','command','offset')
def decode(data):
    if len(data)!=1348: raise ValueError('expected 1348-byte v4 trace symbol')
    written,=struct.unpack_from('<I',data)
    if written>48: raise ValueError('invalid committed row count')
    rows=[]
    for i in range(written):
        row=dict(zip(FIELDS,struct.unpack_from('<7I',data,4+i*28)))
        if row['half']!=174+i: raise ValueError('wrong ELF/layout or incomplete capture')
        if row['consume_tick']:
            row['cq_kit']=(row['command']>>20)&255
            row['cq_voice']=(row['command']>>12)&255
            row['cq_velocity12']=row['command']&4095
        else:
            row['cq_kit']=row['cq_voice']=row['cq_velocity12']=None
        rows.append(row)
    return {'layout':4,'scope':'Independent latest held-key recognition snapshots for raw18/raw22 and last CQ drum consumption this half. No causal pairing or enqueue timestamp; zero recognition means not held/unavailable. Zero consumption means absent or timer-wrap sentinel. Earlier same-half CQ drums are not retained.','written':written,'window_complete':written==48,'rows':rows}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--elf',type=Path,required=True);p.add_argument('--ram',type=Path,required=True)
    p.add_argument('--base',type=lambda x:int(x,0),default=0x1c00000)
    a=p.parse_args();symbols=subprocess.check_output(['readelf','-sW',str(a.elf)],text=True)
    matches=[line.split() for line in symbols.splitlines() if line.split() and line.split()[-1]=='x0x_state_trace']
    if len(matches)!=1: raise ValueError('expected trace symbol in exact ELF')
    addr=int(matches[0][1],16);size=int(matches[0][2]);off=addr-a.base;raw=a.ram.read_bytes()
    if off<0 or off+size>len(raw): raise ValueError('RAM dump does not cover symbol')
    result=decode(raw[off:off+size]);result['symbol_address']=hex(addr)
    print(json.dumps(result,indent=2))
if __name__=='__main__':main()
