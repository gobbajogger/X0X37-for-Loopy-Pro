# SPDX-License-Identifier: GPL-3.0-only
"""Private actual-byte loader replay input. NEVER emits UFW/FWSC or installs.
Usage: real_fixture.py KIT_ZIP APP_ELF APP_FIXTURE_DIR VENDOR_FIXTURE_DIR OUT_DIR
Explicit test key0x1234 only; no device backup/key is consumed or produced.
"""
if not __debug__:
    raise SystemExit("refuse optimized Python: fixture provenance assertions must remain enabled")
from pathlib import Path
import hashlib,json,struct,sys,zipfile,zlib
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
import add_ctl
sys.path.insert(0,str(Path(__file__).resolve().parent.parent/'smc_app_data'))
from seal_fixture import seal
kit,elf,ad,vd,out=map(Path,sys.argv[1:]);assert not out.exists()
assert hashlib.sha256(kit.read_bytes()).hexdigest()=='c9ef5aa79d50189646703a046000c0b3e4e517c00f7df5ecaf4fc13f36a63907'
with zipfile.ZipFile(kit) as z:
    ns=[n for n in z.namelist() if n.endswith('/firmware/x0x-smk37-elite-with-bt.fwsc')];assert len(ns)==1
    raw=z.read(ns[0])
assert hashlib.sha256(raw).hexdigest()=='6f89a299deb2304a89e3094f25ba22dd70214a3ce935c770a6e2ccff39e9b65a'
s=add_ctl._open(add_ctl.logical_of(raw)[1]);am=json.loads((ad/'fixture.json').read_text());vm=json.loads((vd/'manifest.json').read_text())
assert am['installable'] is False and vm['installable'] is False
assert hashlib.sha256(elf.read_bytes()).hexdigest()==am['source_elf_sha256']
app=(ad/'app-NOT-INSTALLABLE.fixture').read_bytes();ui=(ad/'ui-NOT-INSTALLABLE.fixture').read_bytes()
for name,data in (('app-NOT-INSTALLABLE.fixture',app),('ui-NOT-INSTALLABLE.fixture',ui)):
    assert hashlib.sha256(data).hexdigest()==am['private_artifacts'][name]
a=bytearray(app);struct.pack_into('<5I',a,am['descriptor_image_offset'],0,1,0xAB000,8192,0)
sealed,elf_ui,meta=seal(elf.read_bytes(),a);assert sealed==app and elf_ui==ui
if len(vm['materialized_regions'])!=3:
    raise SystemExit("exactly three materialized vendor regions required")
regions=[]
for i,r in enumerate(vm['materialized_regions']):
    b=(vd/r['file']).read_bytes();assert len(b)==r['size'] and hashlib.sha256(b).hexdigest()==r['sha256'] and zlib.crc32(b)==r['crc32']
    assert (r['offset'],len(b))==((0x84000,0x17000),(0xAD000,8192),(0xFE000,4096))[i]
    if i:
        for pos in range(0xF00,len(b),4096):
            assert struct.unpack_from('<H',b,pos+6)[0]!=0x5441,'secondary resembles persistent update record'
    regions.append(b)
assert len(app)<=524000 and len(ui)==8192
# Preserve complete canonical main/config/slot map. Decrypt exact original
# encrypted region; update only app/primary and their existing JLFS CRC fields.
f=bytearray(b'\xff'*0x100000);f[:len(s['flash'])]=s['flash']
main=bytearray(f[0x4000:0x9D000]);add_ctl.pm.sfc(main,0,0x9796e,0,s['key'])
main[0x120:0x80000]=app+b'\xff'*(524000-len(app))
main[0x80000:0x97000]=regions[0]
struct.pack_into('<H',main,34,add_ctl.pm.crc16(main[0x120:0x120+s['slot']]))
struct.pack_into('<H',main,32,add_ctl.pm.crc16(main[34:64]))
struct.pack_into('<H',main,2,add_ctl.pm.crc16(main[32:s['blk']]))
struct.pack_into('<H',main,0,add_ctl.pm.crc16(main[2:32]))
key=0x1234;add_ctl.pm.sfc(main,0,0x9796e,0,key);f[0x4000:0x9D000]=main
# Head fixture replaces only isd key blob, maintaining top-entry CRCs, so the
# actual loader's ldr_chip_key discovers the explicit test key normally.
isd=add_ctl._dec(f[64:96]);at=struct.unpack_from('<I',isd,4)[0];sz=struct.unpack_from('<I',isd,8)[0]
f[at:at+34]=add_ctl.pm.key_blob(key)
struct.pack_into('<H',isd,2,add_ctl.pm.crc16(f[at:at+sz]));struct.pack_into('<H',isd,0,add_ctl.pm.crc16(isd[2:32]));add_ctl.pm.enc(isd,0,32);f[64:96]=isd
f[0xAB000:0xAD000]=ui;f[0xAD000:0xAF000]=regions[1];f[0xFE000:0xFF000]=regions[2]
h=[0x31425058,1,zlib.crc32(b'SMK-37 Elite'),1,0x84000,128,0,0,am['descriptor_flash_offset'],vm['image_id'],0,0];agg=0
for off,n in ((0x4000,0x99000),(0xAB000,8192),(0xAD000,8192),(0xFE000,4096)):
    h.extend((off,n,zlib.crc32(f[off:off+n])));agg=zlib.crc32(f[off:off+n],agg)
h[6]=agg;h[7]=zlib.crc32(struct.pack('<24I',*h));source=f+struct.pack('<24I',*h)
out.mkdir();(out/'source-NOT-INSTALLABLE.fixture').write_bytes(source)
(out/'response-map.json').write_text(json.dumps({'installable':False,'package_generated':False,'fixture_key':key,'source_size':len(source),'source_sha256':hashlib.sha256(source).hexdigest(),'app_sha256':hashlib.sha256(app).hexdigest(),'ui_sha256':hashlib.sha256(ui).hexdigest(),'vendor_region_sha256':[hashlib.sha256(b).hexdigest() for b in regions]},indent=2)+'\n')
print('Actual app/UI/vendor replay source only; explicit fixture key; NO UFW/FWSC')
