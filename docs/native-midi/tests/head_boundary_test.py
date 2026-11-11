#!/usr/bin/env python3
"""Independent offline native head-guard boundaries; never opens MIDI."""
import hashlib, io, json, struct, sys, tempfile
from pathlib import Path
from types import SimpleNamespace
if len(sys.argv) != 4:
    raise SystemExit('usage: head_boundary_test.py REPLAY_ROOT PRIVATE_PACKAGE PRIVATE_REFERENCE')
ROOT=Path(sys.argv[1]).resolve()
sys.path.insert(0,str(ROOT/'tools'))
import fm1_install as m
CANDIDATE=Path(sys.argv[2]).resolve()
REFERENCE=Path(sys.argv[3]).resolve()
raw=CANDIDATE.read_bytes(); prod, image=m.load_package(CANDIDATE,False)
ref=REFERENCE.read_bytes()[:m.HEAD_END]
rows=[]
def test(name, condition, detail=''):
    rows.append({'name':name,'pass':bool(condition),'detail':detail})
    print(('PASS ' if condition else 'FAIL ')+name+(' '+detail if detail else ''))
def parts(im):
    h=bytearray(m._dec(im[:64])); entries=[]
    for i in range(int.from_bytes(h[8:10],'little')):
        e=bytearray(m._dec(im[64+80*i:64+80*(i+1)])); off,size=struct.unpack_from('<II',e,8)
        entries.append((e,bytearray(im[off:off+size])))
    return h,entries
def change(index, fn):
    h,es=parts(image); fn(es[index][1])
    # Repack file entries and data, preserving names/types and all CRCs.
    end=0x400; data=bytearray(end)
    encrypted=[]
    for e,payload in es:
        struct.pack_into('<H',e,4,m._crc16(payload));struct.pack_into('<II',e,8,end,len(payload))
        encrypted.append(m._dec(e)); data.extend(payload);end+=len(payload)
    ls=b''.join(encrypted);struct.pack_into('<H',h,2,m._crc16(ls));struct.pack_into('<H',h,0,m._crc16(h[2:]))
    data[:64]=m._dec(h);data[64:64+len(ls)]=ls
    assert m.package_damage(data) is None
    return bytes(data)
def outer(loader):
    struct.pack_into('<H',loader,2,m._crc16(loader[32:]));struct.pack_into('<H',loader,0,m._crc16(loader[2:32]))
def inner(loader):
    struct.pack_into('<H',loader,32,m._crc16(loader[34:64]));outer(loader)
with tempfile.TemporaryDirectory(prefix='head-review-') as td:
    td=Path(td)
    def guard(name, im=image, product=prod, reference=ref, force=False, want=11, exception=False):
        pkg=td/'candidate.fwsc';rp=td/'ref.bin'
        pkg.write_bytes(m.add_ctl.fwsc_of(im,product))
        if reference is not None:rp.write_bytes(reference)
        else:
            rp.unlink(missing_ok=True)
        out=io.StringIO()
        rc=m.head_guard(SimpleNamespace(package=str(pkg),head_ref=str(rp),force=force),out=out,env={})
        result=out.getvalue()
        test(name,rc==want and (not exception or 'exact qualified Elite' in result),f'rc={rc}')
        return result
    test('candidate raw identity',len(raw)==651396 and hashlib.sha256(raw).hexdigest()=='d14877d6592e6dc158460e960ac5aefd1b2b775588b9f896d3c83f672d61c86c')
    test('reference differs without exception',not m.head_compare(image,str(REFERENCE))[0])
    guard('exact candidate + original reference',want=0,exception=True)
    guard('Pro identity wrapping identical bytes',product='SMK-37 Pro_900')
    guard('FM-1 identity wrapping identical bytes',product='FM-1_900')
    guard('non-900 Elite identity wrapping identical bytes',product='SMK-37 Elite_901')
    guard('short reference',reference=ref[:-1])
    guard('missing explicit reference',reference=None)
    r=bytearray(ref);r[32]^=1
    guard('reference JLFS entry CRC broken',reference=r)
    # Find validated key entry and produce wrong-key but CRC-correct blob.
    for off in range(32,0x400,32):
        e=m._dec(ref[off:off+32])
        if e[16:31]==b'isd_config.ini\0':
            at=struct.unpack_from('<I',e,4)[0];break
    else: raise AssertionError('reference lacks key entry')
    key=m.fm1pkg_make.chipkey_decode(ref[at:at+32])
    r=bytearray(ref);r[at:at+34]=m.fm1pkg_make.key_blob(key^1)
    test('wrong key fixture is CRC-correct',m._crc16(r[at:at+32])==int.from_bytes(r[at+32:at+34],'little'))
    guard('foreign reference key with valid blob CRC',reference=r)
    r=bytearray(ref);r[at]^=1
    guard('reference key blob CRC broken',reference=r)
    r=bytearray(ref)
    e=bytearray(m._dec(r[64:96]));struct.pack_into('<I',e,4,m.HEAD_END-2);struct.pack_into('<H',e,0,m._crc16(e[2:]));r[64:96]=m._dec(e)
    guard('reference key blob out of bounds',reference=r)
    def mutate_at(at,v=1):
        def fn(b):b[at]^=v
        return fn
    guard('payload altered with repaired package CRC',im=change(0,mutate_at(0x5000)))
    guard('encrypted app header altered with repaired package CRC',im=change(0,mutate_at(0x4000)))
    def body_change(b):b[-1]^=1;outer(b)
    guard('loader changed but all loader/package CRCs repaired',im=change(1,body_change))
    def unknown(b):
        at=b.find(m.LOADER_MARK);b[at:at+len(m.LOADER_MARK)]=b'UNKNOWN-LOADER-1\0';outer(b)
    unknown_image=change(1,unknown)
    guard('unknown loader with force and repaired CRCs',im=unknown_image,force=True)
    guard('unknown loader historical equality retained',im=unknown_image,reference=m.package_head(unknown_image),force=True,want=0)
    def official(b):
        b.extend(b'\0'*(0x4de1+32-len(b)));struct.pack_into('<HHI',b,0,0xEBAA,0x5881,0x20);struct.pack_into('<I',b,8,0x4de1)
    official_image=change(1,official)
    guard('synthetic official loader tuple mismatch refuses',im=official_image,force=True)
    guard('synthetic official tuple equality retained',im=official_image,reference=m.package_head(official_image),force=True,want=0)
    def bad_inner(b):b[34]^=1;outer(b)
    guard('malformed inner loader header',im=change(1,bad_inner))
    def bad_entry(b):struct.pack_into('<I',b,0x28,0x01c0a801);inner(b)
    guard('wrong loader entry address valid CRCs',im=change(1,bad_entry))
    def bounds(b):struct.pack_into('<I',b,0x24,0x14001);inner(b)
    guard('loader load bounds exceeded valid CRCs',im=change(1,bounds))
    def attr(b):b[12]=0x40;outer(b)
    guard('loader attribute incorrect valid CRCs',im=change(1,attr))
    def length(b):struct.pack_into('<I',b,8,len(b)-31);outer(b)
    guard('loader exact length wrong valid CRCs',im=change(1,length))
    pkg=td/'bad.fwsc';pkg.write_bytes(b'invalid');out=io.StringIO()
    rc=m.head_guard(SimpleNamespace(package=str(pkg),head_ref=str(REFERENCE),force=False),out=out,env={})
    test('invalid package no false HEAD OK',rc==11 and 'REFUSED' in out.getvalue(),f'rc={rc}')
    out=io.StringIO();rc=m.main([str(CANDIDATE),'--head-check','--head-ref',str(REFERENCE)],out=out)
    test('CLI head-check positive is offline',rc==0 and 'HEAD OK' in out.getvalue(),f'rc={rc}')
result={'updater_sha256':hashlib.sha256((ROOT/'tools/fm1_install.py').read_bytes()).hexdigest(),'tests':rows,'passed':sum(x['pass'] for x in rows),'total':len(rows),'hardware_io':False}
# Results are printed only; the private inputs and original review remain unchanged.
print(f"{result['passed']}/{result['total']} passed")
sys.exit(0 if result['passed']==result['total'] else 1)
