#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Native transaction replay of the exact final DEV package. No device access."""
from pathlib import Path
import os
import subprocess
import sys
import smc_vendor_bundle as bundle

if len(sys.argv)!=3:raise SystemExit('usage: smc_vendor_replay.py PACKAGE COMPONENT_BUILD')
package=Path(sys.argv[1]).resolve();root=Path(sys.argv[2]).resolve()
source,ota=bundle.unpack(package.read_bytes())
out=root/'package-replay';out.mkdir()
(out/'source-NOT-INSTALLABLE.fixture').write_bytes(source)
(out/'ota.bin').write_bytes(ota)
repo=Path(__file__).resolve().parents[1]
app=root/'app'
args=['cc','-O2','-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-function',
      '-DBP_OTA_SOURCE="'+str(app/'firmware/src/ota.c')+'"',
      '-DBP_LDR_SOURCE="'+str(app/'firmware/loader/ldr_core.c')+'"',
      '-I'+str(app/'firmware/hal'),str(repo/'tools/studies/smc_bundle_loader/wire_replay.c')]
subprocess.run(args+['-o',str(out/'replay')],check=True)
subprocess.run([str(out/'replay'),str(out/'source-NOT-INSTALLABLE.fixture'),str(out/'ota.bin')],check=True)
subprocess.run(args+['-O1','-g','-fsanitize=address,undefined','-o',str(out/'replay-sanitized')],check=True)
env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
subprocess.run([str(out/'replay-sanitized'),str(out/'source-NOT-INSTALLABLE.fixture'),str(out/'ota.bin')],check=True,env=env)
print('Exact DEV package payload and paired loader: native and ASan/UBSan transaction replay PASS')
