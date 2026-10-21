# SPDX-License-Identifier: GPL-3.0-only
"""Verify exact approved kit and produce PRIVATE native input, never a package.
Usage: rollback_fixture.py APPROVED_816_INSTALLER_ZIP NEW_PRIVATE_OUTPUT
"""
from pathlib import Path
import hashlib,sys,zipfile,struct,zlib
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
import add_ctl
kit=Path(sys.argv[1]).read_bytes()
assert hashlib.sha256(kit).hexdigest()=='c9ef5aa79d50189646703a046000c0b3e4e517c00f7df5ecaf4fc13f36a63907'
with zipfile.ZipFile(sys.argv[1]) as z:
    names=[n for n in z.namelist() if n.endswith('/firmware/x0x-smk37-elite-with-bt.fwsc')];assert len(names)==1
    raw=z.read(names[0])
assert hashlib.sha256(raw).hexdigest()=='6f89a299deb2304a89e3094f25ba22dd70214a3ce935c770a6e2ccff39e9b65a'
product,logical=add_ctl.logical_of(raw);assert product=='SMK-37 Elite_900'
s=add_ctl._open(logical); app=s['area'][0x120:0x120+548480];ctl=s['area'][0x87000:0x87000+65524]
assert hashlib.sha256(app).hexdigest()=='4b7c76e4bdeca3d8b1ce366e868406b8d554b0ae7d87beb282bd99254dcd0ec9'
assert hashlib.sha256(ctl).hexdigest()=='9d6855e47791b3cab698af95d9625c35d2ea7b01d1fa7216e72ea622683a8245'
assert add_ctl.ctl_problem(ctl) is None # actual ABI2 header68 + CRC/IRQ/RAM/entry checks
plain=bytearray(s['flash'][0x4000:0x9D000]);add_ctl.pm.sfc(plain,0,len(plain),0,s['key'])
cfg=s['blk'];assert plain[cfg+16:cfg+20]==b'cfg\0'
crypt=cfg+struct.unpack_from('<I',plain,cfg+8)[0]
assert crypt==0x9796e
canonical=bytearray(s['flash'][0x4000:0x9D000]);add_ctl.pm.sfc(canonical,0,crypt,0,s['key'])
assert zlib.crc32(canonical)==0x53f84577
# Logical rollback intentionally erases only known-unused secondary extents.
# This is NOT an original per-device backup or byte-identical rollback.
b=bytearray(b'\xff'*(0x100000+96));b[:len(s['flash'])]=s['flash']
for o,n in ((0xAB000,8192),(0xAD000,8192),(0xFE000,4096)):b[o:o+n]=b'\xff'*n
h=[0x31425058,1,zlib.crc32(b'SMK-37 Elite'),2,0x8B000,128,0,0,0,0x0816E0D3,0,0];agg=0
for o,n in ((0x4000,0x99000),(0xAB000,8192),(0xAD000,8192),(0xFE000,4096)):
    h.extend((o,n,zlib.crc32(b[o:o+n])));agg=zlib.crc32(b[o:o+n],agg)
h[6]=agg;data=struct.pack('<24I',*h);h[7]=zlib.crc32(data);b[0x100000:]=struct.pack('<24I',*h)
out=Path(sys.argv[2]);assert not out.exists();out.write_bytes(struct.pack('<I',s['key'])+b)
print('Verified exact baseline app548480/ctl65524; canonical main CRC53f84577; private native input only')
