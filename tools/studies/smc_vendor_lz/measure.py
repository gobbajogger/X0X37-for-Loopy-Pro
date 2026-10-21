# SPDX-License-Identifier: GPL-3.0-only
# PRIVATE outputs include vendor bytes: never add output directory to Git/bundles.
from pathlib import Path
import hashlib,json,os,subprocess,sys
if len(sys.argv)!=4:
    raise SystemExit('usage: measure.py PRIVATE_VENDOR_ELF TOOLCHAIN_ROOT PRIVATE_OUTPUT_DIR')
elf,tc,out=map(lambda s:Path(s).resolve(),sys.argv[1:]);out.mkdir(parents=True,exist_ok=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
expected='8096128a2f9c513c29a5e5f89ee0e41610295261139a7c1d1339213ab27b64c8'
if sha(elf)!=expected:raise SystemExit('ELF differs from scoped-CPU measurement; review new input first')
for name in ('ramtext','data'):
    subprocess.run([str(tc/'common/bin/objcopy'),'-O','binary','-j','.'+name,str(elf),str(out/(name+'.bin'))],check=True)
(out/'combined.bin').write_bytes((out/'ramtext.bin').read_bytes()+(out/'data.bin').read_bytes())
subprocess.run([os.environ.get('CC','cc'),'-std=c11','-O2','-Wall','-Wextra','-Werror',str(Path(__file__).with_name('probe.c')),'-o',str(out/'probe')],check=True)
result={'installable':False,'elf_sha256':sha(elf),'variants':{}}
for name in ('ramtext','data','combined'):
    raw,packed=out/(name+'.bin'),out/(name+'.lz')
    subprocess.run([str(out/'probe'),str(raw),str(packed)],check=True)
    result['variants'][name]={'raw':raw.stat().st_size,'packed':packed.stat().st_size,'raw_sha256':sha(raw),'packed_sha256':sha(packed)}
(out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
