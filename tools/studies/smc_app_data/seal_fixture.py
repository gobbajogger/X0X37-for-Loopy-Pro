#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Seal app-owned UI descriptor in private validation fixtures, NOT a firmware kit."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib
APP=0x02000120
UI=0x020A7000
LIMIT=524000

def seal(elf, image):
    def read(fmt, off):
        if off<0 or off+struct.calcsize(fmt)>len(elf): raise ValueError('truncated ELF')
        return struct.unpack_from(fmt,elf,off)
    def span(off,n):
        if off<0 or n<0 or off+n>len(elf): raise ValueError('truncated ELF section')
        return elf[off:off+n]
    def string(b,n):
        if n>=len(b) or b'\0' not in b[n:]: raise ValueError('bad ELF string')
        return b[n:b.index(0,n)].decode('ascii')
    if elf[:7]!=b'\x7fELF\x01\x01\x01' or read('<HH',16)!=(2,241): raise ValueError('pi32v2 executable required')
    phoff,shoff=read('<II',28)
    psz,pnum,ssz,snum,sstr=read('<HHHHH',42)
    if psz!=32 or ssz!=40 or not pnum or not snum or sstr>=snum: raise ValueError('unsupported ELF tables')
    ph=[read('<8I',phoff+i*psz) for i in range(pnum)]
    sh=[read('<10I',shoff+i*ssz) for i in range(snum)]
    names=span(sh[sstr][4],sh[sstr][5]); sections={}
    for s in sh:
        name=string(names,s[0])
        if name in sections: raise ValueError('duplicate section')
        sections[name]=s
    ui=sections.get('.app_ui')
    if not ui or ui[1]!=1 or ui[2]&4 or ui[3]!=UI or ui[5]!=8192: raise ValueError('exact immutable UI region required')
    data=span(ui[4],ui[5])
    if not 0<len(image)<=LIMIT or len(image)%4: raise ValueError('app partition overflow/alignment')
    reconstructed=bytearray(b'\xff'*len(image)); covered=set()
    for name,s in sections.items():
        _,typ,flags,vma,off,n,*_=s
        if typ!=1 or not flags&2 or not n or name=='.app_ui':continue
        ps=[p for p in ph if p[0]==1 and p[1]<=off and off+n<=p[1]+p[4]]
        if len(ps)!=1:raise ValueError('ambiguous section load address')
        lma=ps[0][3]+off-ps[0][1]; pos=lma-APP
        if pos<0 or pos+n>len(image):raise ValueError('section outside app image')
        new=set(range(pos,pos+n))
        if covered&new:raise ValueError('overlapping app sections')
        covered|=new;reconstructed[pos:pos+n]=span(off,n)
    if reconstructed!=image:raise ValueError('app binary does not exactly match ELF load bytes')
    symbols={}
    for s in sh:
        if s[1]!=2:continue
        if s[9]!=16 or s[5]%16 or s[6]>=snum:raise ValueError('bad symbol table')
        strs=span(sh[s[6]][4],sh[s[6]][5])
        for off in range(s[4],s[4]+s[5],16):
            name,value,size,info,other,index=read('<IIIBBH',off)
            if name and index:symbols[string(strs,name)]=(value,size)
    desc=symbols.get('app_ui_descriptor')
    if not desc or desc[1]!=20:raise ValueError('exact app descriptor symbol required')
    pos=desc[0]-APP
    if pos<0 or pos+20>len(image) or not set(range(pos,pos+20))<=covered:raise ValueError('descriptor outside backed app')
    old=struct.unpack_from('<5I',image,pos)
    if old!=(0,1,0xAB000,8192,0):raise ValueError('expected unsealed app descriptor')
    out=bytearray(image);crc=zlib.crc32(data)&0xffffffff
    struct.pack_into('<5I',out,pos,0x55493058,1,0xAB000,8192,crc)
    return out,data,{'descriptor_vma':desc[0],'descriptor_image_offset':pos,'descriptor_flash_offset':desc[0]-0x01FFC000,
                     'app_size':len(out),'sealed_plaintext_app_crc32':zlib.crc32(out)&0xffffffff,'ui_crc32':crc}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--elf',type=Path,required=True);p.add_argument('--app',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    a=p.parse_args()
    if a.out.exists():p.error('output must be new')
    try:app,ui,meta=seal(a.elf.read_bytes(),a.app.read_bytes())
    except (ValueError,UnicodeError) as e:p.error(str(e))
    a.out.mkdir(parents=True)
    files={'app-NOT-INSTALLABLE.fixture':app,'ui-NOT-INSTALLABLE.fixture':ui}
    meta.update(installable=False,bundle_generated=False,source_elf_sha256=hashlib.sha256(a.elf.read_bytes()).hexdigest(),source_app_sha256=hashlib.sha256(a.app.read_bytes()).hexdigest())
    meta['private_artifacts']={}
    for name,b in files.items():
        (a.out/name).write_bytes(b);meta['private_artifacts'][name]=hashlib.sha256(b).hexdigest()
    (a.out/'fixture.json').write_text(json.dumps(meta,indent=2)+'\n')
    (a.out/'NON-INSTALLABLE.txt').write_text('App descriptor validation fixture only. No complete updater, live-memory or hardware admission. NEVER INSTALL.\n')
    print(json.dumps(meta,indent=2))
if __name__=='__main__':main()
