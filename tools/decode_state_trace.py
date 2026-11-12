#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Decode the exact ELF's X0X_DIAG_STATE_TRACE RAM snapshot (layout v2)."""
import argparse
import json
from pathlib import Path
import struct
import subprocess

FIELDS=('half','late','phase909','events','count','drums','clock_next','steps','plan','prior_cost','best_cost','decision')
def decode(data):
    if len(data)!=2308: raise ValueError('expected 2308-byte v2 trace symbol')
    written,=struct.unpack_from('<I',data)
    if written>48: raise ValueError('invalid committed row count')
    rows=[]
    for i in range(written):
        row=dict(zip(FIELDS,struct.unpack_from('<12I',data,4+i*48)))
        if row['half']!=174+i: raise ValueError('nonsequential half index: wrong ELF/layout or incomplete capture')
        row['track_steps']=[(row['steps']>>(j*6))&63 for j in range(5)]
        row['drum_requests']=[(j//11,j%11) for j in range(22) if row['drums']&(1<<j)]
        row['planner_comparison']={'prior_mask':row['decision']&255,'best_mask':(row['decision']>>8)&255,'stretch_frames':row['decision']>>16}
        row['phase909_samples_q8']=row['phase909']
        row['clock_samples_q8']=row['clock_next']
        rows.append(row)
    return {'layout':2,'written':written,'window_complete':written==48,'rows':rows}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--elf',type=Path,required=True);p.add_argument('--ram',type=Path,required=True)
    p.add_argument('--base',type=lambda x:int(x,0),default=0x1c00000)
    a=p.parse_args()
    symbols=subprocess.check_output(['readelf','-sW',str(a.elf)],text=True)
    matches=[line.split() for line in symbols.splitlines() if line.split() and line.split()[-1]=='x0x_state_trace']
    if len(matches)!=1: raise ValueError('expected one trace symbol in this exact ELF; was diagnostic enabled?')
    addr=int(matches[0][1],16);size=int(matches[0][2]);off=addr-a.base
    raw=a.ram.read_bytes()
    if off<0 or off+size>len(raw): raise ValueError('RAM dump does not cover ELF trace symbol')
    result=decode(raw[off:off+size]);result['symbol_address']=hex(addr)
    print(json.dumps(result,indent=2))
if __name__=='__main__':main()
