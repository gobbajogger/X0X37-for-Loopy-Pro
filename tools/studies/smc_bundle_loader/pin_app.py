# SPDX-License-Identifier: GPL-3.0-only
"""Extract the descriptor from the actual final ELF; never trust bundle offset.
Usage: pin_app.py FINAL_APP_ELF SEALED_APP_BIN OUTPUT_bp_pin.h. No firmware/package output.
"""
from pathlib import Path
import struct,sys,hashlib,zlib
sys.path.insert(0,str(Path(__file__).resolve().parent.parent/"smc_app_data"))
from seal_fixture import seal
p=Path(sys.argv[1]); b=p.read_bytes()
if b[:6]!=b'\x7fELF\x01\x01':raise SystemExit('expected ELF32 little endian')
shoff=struct.unpack_from('<I',b,32)[0]; ent,n=struct.unpack_from('<HH',b,46)
sections=[struct.unpack_from('<10I',b,shoff+i*ent) for i in range(n)]
found=[]
for s in sections:
    if s[1]!=2:continue
    st=sections[s[6]]; strings=b[st[4]:st[4]+st[5]]
    for at in range(s[4],s[4]+s[5],s[9]):
        name,val,size,info,other,idx=struct.unpack_from('<IIIBBH',b,at)
        if strings[name:].split(b'\0',1)[0]==b'app_ui_descriptor':found.append((val,size,idx))
if len(found)!=1:raise SystemExit('one final app_ui_descriptor required')
val,size,idx=found[0]; off=val-0x02000000+0x4000
if size!=20 or idx==0 or not 0x4000<=off<=0x84000-20 or off&3:raise SystemExit('descriptor outside retained primary')
app=Path(sys.argv[2]).read_bytes(); start=0x4120
if not 20<=len(app)<=0x84000-start:raise SystemExit('app exceeds fixed retained primary')
if not 0<=off-start<=len(app)-20:raise SystemExit('descriptor not within app bytes')
d=struct.unpack_from('<5I',app,off-start)
if d[:4]!=(0x55493058,1,0xAB000,8192):raise SystemExit('sealed descriptor required')
unsealed=bytearray(app)
struct.pack_into('<5I',unsealed,off-start,0,1,0xAB000,8192,0)
resealed,ui,meta=seal(b,unsealed)
if resealed!=app:raise SystemExit('sealed app differs from proven ELF + descriptor')
Path(sys.argv[3]).write_text('/* Final ELF SHA256 '+hashlib.sha256(b).hexdigest()+'; sealed app SHA256 '+hashlib.sha256(app).hexdigest()+' */\n#define BP_APPDESC_OFF 0x%xu\n#define BP_APP_OFF 0x%xu\n#define BP_APP_SIZE %du\n#define BP_APP_CRC 0x%xu\n'%(off,start,len(app),zlib.crc32(app)))
print('descriptor physical 0x%x; app %d bytes CRC %08x'%(off,len(app),zlib.crc32(app)))
