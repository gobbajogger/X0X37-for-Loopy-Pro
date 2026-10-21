# SPDX-License-Identifier: GPL-3.0-only
"""Private-fixture provenance regressions; writes only temporary outputs."""
import subprocess,sys,tempfile,struct
from pathlib import Path
elf=Path(sys.argv[1]).read_bytes();app=Path(sys.argv[2]).read_bytes()
script=Path(__file__).with_name('pin_app.py')
with tempfile.TemporaryDirectory() as td:
    d=Path(td); ep=d/'app.elf';ap=d/'app.bin';out=d/'pin.h'
    def run(e,a,ok):
        ep.write_bytes(e);ap.write_bytes(a)
        r=subprocess.run([sys.executable,str(script),str(ep),str(ap),str(out)],capture_output=True)
        assert (r.returncode==0)==ok,r.stderr.decode()
    run(elf,app,True)
    a=bytearray(app);a[1234]^=1;run(elf,a,False)
    e=bytearray(elf);shoff=struct.unpack_from('<I',e,32)[0];sz,num=struct.unpack_from('<HH',e,46)
    sh=[struct.unpack_from('<10I',e,shoff+i*sz) for i in range(num)]
    for s in sh:
        if s[1]!=2:continue
        st=sh[s[6]];strings=e[st[4]:st[4]+st[5]]
        for at in range(s[4],s[4]+s[5],s[9]):
            name=struct.unpack_from('<I',e,at)[0]
            if strings[name:].split(b'\0',1)[0]==b'app_ui_descriptor':
                struct.pack_into('<I',e,at+4,0x02000000);break
    run(e,app,False)
    run(elf,app[:-4],False)
print('ELF/sealed-app provenance: valid, changed byte, below-app symbol, truncation PASS')
