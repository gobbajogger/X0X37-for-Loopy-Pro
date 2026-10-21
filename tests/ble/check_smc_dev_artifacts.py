#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Verify actual checked packages, then reject fully reframed alternate payloads.
Usage: check_smc_dev_artifacts.py CHECKED_DIRECTORY
Writes disposable copies only, never edits evidence or opens a device.
"""
import copy,json,shutil,struct,sys,tempfile,zlib
from pathlib import Path
repo=Path(__file__).resolve().parents[2];sys.path.insert(0,str(repo/'tools'))
import smc_vendor_bundle as b
import smc_vendor_dev_validate as v
root=Path(sys.argv[1]).resolve()
for name in ('record.json','rollback-record.json'):
    record=json.loads((root/name).read_text())
    v.verify(repo,record['commit'],root/name,root/record['artifacts']['package']['file'])
r=json.loads((root/'record.json').read_text());bind=r['loader_binding']
f,ota=b.unpack((root/r['artifacts']['package']['file']).read_bytes())
m,key,blk=b._main(f);app=bytes(m[0x120:0x120+bind['app_size']]);ui=f[0xAB000:0xAD000]
regions=[bytes(m[0x80000:0x97000]),f[0xAD000:0xAF000],f[0xFE000:0xFF000]]
# Restore the exact app-only constructor template, not an already populated slot.
blank=bytearray(m);blank[0x120+len(app):0x97520]=b'\xff'*(0x97400-len(app))
b._recrc(blank,blk);b.pm.sfc(blank,0,b.CRYPT_SIZE,0,key)
template0=bytearray(f[:0x9D000]);template0[0x4000:0x9D000]=blank
# A different structurally valid vendor image, with every internal CRC repaired.
rs=list(regions);rs[1]=bytes([rs[1][0]^1])+rs[1][1:]
h=bytearray(rs[0]);used=struct.unpack_from('<I',h,88)[0]
struct.pack_into('<I',h,96,zlib.crc32(rs[1][:used]));struct.pack_into('<I',h,116,0)
struct.pack_into('<I',h,116,zlib.crc32(h[:128]));rs[0]=bytes(h)
alternate=b.build_bundle(template0,app,ui,rs,ota,descriptor_off=bind['descriptor_off'],loader_binding=bind)
# Different canonical padding/template data, not an app/loader pin violation.
template=bytearray(template0);template[0x9C000]^=1
canonical=b.build_bundle(template,app,ui,regions,ota,descriptor_off=bind['descriptor_off'],loader_binding=bind)
with tempfile.TemporaryDirectory() as tmp:
    out=Path(tmp)
    for name in r['logs']:shutil.copyfile(root/(name+'.log'),out/(name+'.log'))
    head=r['artifacts']['head_export']['file'];shutil.copyfile(root/head,out/head)
    for label,raw,rebind_metadata in (('alternate vendor',alternate,True),('canonical main',canonical,False)):
        d=copy.deepcopy(r);p=out/'changed.fwsc';p.write_bytes(raw)
        d['artifacts']['package']['sha256']=b.sha(raw)
        meta=b.validate_bundle(raw,loader_binding=bind)
        if rebind_metadata:d['smc_vendor']=meta
        q=out/'record.json';q.write_text(json.dumps(d))
        try:v.verify(repo,r['commit'],q,p)
        except ValueError as exc:
            expected='reviewed pinned ELF' if rebind_metadata else 'paired component identity'
            if expected not in str(exc):raise RuntimeError(label+' refused for wrong reason: '+str(exc))
        else:raise RuntimeError(label+' accepted after repaired framing/hash')
print('Actual forward/rollback schema3 PASS; alternate valid vendor and canonical-main payloads rejected')
