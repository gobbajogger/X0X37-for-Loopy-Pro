#!/usr/bin/env python3
"""Portable app-only measurement; private output, no packages/devices/publication.
Usage: JIELI_TOOLCHAIN=<toolchain> X0X_DEV_PADCAL=1 python3 reproduce.py CHECKOUT GENERATED_HEADERS OUTPUT elite|pro [--diagnostic]
Use exact source checkout and the four hash-verified generated headers in receipt.
Run in pinned Linux image with network disabled. --diagnostic only relaxes private
linker MEMORY length to inspect an oversized image; build.check still fails the
real board budget. This reproducer compiles all app TUs (no cached objects).
"""
from pathlib import Path
import hashlib, json, os, sys
repo, gen, out = map(lambda s: Path(s).resolve(), sys.argv[1:4])
model=sys.argv[4]; diagnostic='--diagnostic' in sys.argv[5:]
assert model in ('elite','pro') and not out.exists()
assert os.environ.get('X0X_DEV_PADCAL')=='1'
out.mkdir(parents=True);os.chdir(repo);sys.path.insert(0,str(repo/'tools'))
import build as b
b.fm1pkg_make.set_board('smk37');b.BOARD='smk37';b.APP_SLOT=b.fm1pkg_make.APP_SLOT;b.APP_CODE_MAX=b.fm1pkg_make.APP_CODE_MAX
b.OUT=out;b.PRODUCT=('SMK-37 Elite' if model=='elite' else 'SMK-37 Pro')+'_900'
b.BOARD_DEFS['smk37']=['-DBOARD_SMK37=1']+(['-DSMK_MODEL="SMK-37 Elite"'] if model=='elite' else [])
real=b.tc
if diagnostic:
 ld=out/'diagnostic.ld';ld.write_text((repo/'firmware/app.ld').read_text().replace('LENGTH = 0x8DFBC','LENGTH = 0x90000'))
def tc(tool,*args):
 args=[str(x) for x in args]
 args=[str('-I'+str(gen)) if x=='-Ibuild/gen' else x for x in args]
 if diagnostic and tool=='pi32v2/bin/ld':args=[str(ld) if x==str(out/'app.ld') else x for x in args]
 return real(tool,*args)
b.tc=tc
try:
 img,syms,dis,rt=b.build_app();errors,notes=b.check(img,syms,dis,rt)
 result={'model':model,'diagnostic_only':diagnostic,'installable':False,'bytes':len(img),'code_free':b.APP_CODE_MAX-len(img),'errors':errors,'notes':notes,'sha256':hashlib.sha256(img).hexdigest()}
 (out/'measurement.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));sys.exit(bool(errors))
except BaseException as e:
 (out/'outcome.txt').write_text(repr(e)+'\n');raise
