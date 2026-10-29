#!/usr/bin/env python3
"""Private/dev only. Pinned original metadata -> inactive typed factory header.
No original binary reads, network, runtime activation, or public publishing.
"""
import argparse, hashlib, json, math, os, pathlib, re, struct, tempfile
LEGACY_PIN='652d3e92d4b22765f03cdccb8bc6f9b48a8ab41a415b20e1873f4394d904eb14'
PIN='17c5913d8902963364d0fbcbcd741f94720eb180e6eb9d8532430b0b6877a42f'
LIMITS=[1,127,127,127,127,127,1,127,127,127,127,1,127,127,127,127,127,1]
NEW_LIMITS=[2 if i==0 else 3 if i in (6,11) else v for i,v in enumerate(LIMITS)]
DEFAULT_TAIL=[0,0,127,0,36]+[0]*9
MAP={0:[0,2,1],6:[0,2,3,1],11:[0,2,3,1],13:[0,2,4,6,1,3,5,7]}
THRESH={0:[1048575999,1061158912],6:[1042983594,1056964608,1062557013],11:[1042983594,1056964608,1062557013],13:[1032997156,1046179255,1052171118,1056964608,1059361353,1061758098,1064154843]}
def bits(value):
    if isinstance(value,bool) or not isinstance(value,(int,float)) or not math.isfinite(value) or not 0<=value<=1:raise ValueError('invalid normalized value')
    return struct.unpack('<I',struct.pack('<f',value))[0] if value else 0
def f32(value):return struct.unpack('<f',struct.pack('<I',bits(value)))[0]
def ordinal(value,id):return sum(bits(value)>=b for b in THRESH[id])
def pot(value,maximum):return (round(f32(value)*255)*maximum+127)//255

def build(doc, legacy):
    records=doc['original_resource_bank']['records']
    if len(records)!=30 or len(legacy)!=30:raise ValueError('row count')
    names=[r['name'] for r in records]
    if len(set(names))!=30 or set(names)!=set(legacy):raise ValueError('names')
    checked={}
    for r in records:
        name=r['name'];v=r['parameters']
        if not isinstance(name,str) or not name or len(name.encode('ascii'))>=24 or len(v)!=18:raise ValueError('record shape')
        if not r['exact_original_NKS_vector_matches']:raise ValueError('missing NKS match')
        b=[bits(x) for x in v]; o=[ordinal(v[i],i) for i in MAP]
        cell=[pot(v[i],NEW_LIMITS[i]) for i in range(18)]
        for id,k in zip(MAP,o):
            cell[id]=MAP[id][k] if id!=13 else (MAP[id][k]*127+3)//7
        checked[name]=(cell,o,b)
    lines=['/* SPDX-License-Identifier: GPL-3.0-only */','/* GENERATED PRIVATE/DEV ORIGINAL METADATA: do not publish. Inactive until integrated. */','#pragma once','#include "miami_factory_types.h"',f'/* reference_sha256 {PIN} */','static const miami_factory_typed_t MIAMI_FACTORY_TYPED[30] = {']
    for name in legacy:
        cell,o,b=checked[name]
        digest=hashlib.sha256(struct.pack('<18I',*b)).hexdigest()
        lines += [f'/* {name}: float32_le_sha256 {digest}; bits '+','.join(f'{x:08x}' for x in b)+' */',f'{{1,1,18,{json.dumps(name)},{{'+','.join(map(str,o))+'},{'+','.join(map(str,cell))+'}},']
    lines+=['};','/* Frozen ff4606 full legacy product cells (including tail/default/unused bytes). */','static const uint8_t MIAMI_FACTORY_LEGACY[30][32] = {']
    for name,raw in legacy.items():
        if len(raw)!=22 or any(not isinstance(x,int) or not 0<=x<=255 for x in raw):raise ValueError('legacy bytes')
        cell=[(raw[i]*LIMITS[i]+127)//255 for i in range(18)]+DEFAULT_TAIL
        lines+=['{'+','.join(map(str,cell))+'},']
    return '\n'.join(lines+['};',''])

def generate(reference,source,out):
    raw=reference.read_bytes()
    if hashlib.sha256(raw).hexdigest()!=PIN:raise ValueError('reference pin mismatch')
    if hashlib.sha256(source.read_bytes()).hexdigest()!=LEGACY_PIN:raise ValueError('legacy source pin mismatch')
    source_text=source.read_text().split('static const factory_preset_t miami_presets[30]')[1].split('};')[0]
    rows=re.findall(r'\{ "([^"]+)", 18, \{ ([^}]+)',source_text)
    legacy={name:list(map(int,values.split(','))) for name,values in rows}
    if len(rows)!=30 or len(legacy)!=30:raise ValueError('legacy names')
    output=build(json.loads(raw),legacy) # validate EVERYTHING before file creation
    out.parent.mkdir(parents=True,exist_ok=True)
    fd,tmp=tempfile.mkstemp(dir=out.parent,prefix='.miami-')
    try:
        with os.fdopen(fd,'w') as f:f.write(output)
        os.replace(tmp,out)
    finally:
        if os.path.exists(tmp):os.unlink(tmp)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('reference',type=pathlib.Path);p.add_argument('legacy_source',type=pathlib.Path);p.add_argument('out',type=pathlib.Path);a=p.parse_args();generate(a.reference,a.legacy_source,a.out)
