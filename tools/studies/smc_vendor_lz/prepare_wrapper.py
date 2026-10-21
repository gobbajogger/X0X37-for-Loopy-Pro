# SPDX-License-Identifier: GPL-3.0-only
# Add a retained, uncalled sizing root to an existing NON-INSTALLABLE app export.
from pathlib import Path
import shutil
import sys
if len(sys.argv)!=2:
    raise SystemExit('usage: prepare_wrapper.py DISPOSABLE_PREPARED_APP_EXPORT')
p=Path(sys.argv[1]).resolve()
if (p/'.git').exists() or 'NON-INSTALLABLE' not in (p/'tools/build.py').read_text():
    raise SystemExit('requires non-installable archive export, not a worktree')
shutil.copyfile(Path(__file__).with_name('load_probe.h'),p/'firmware/src/app/vendor_lz_probe.h')
f=p/'firmware/src/x0x.c';s=f.read_text();old='#include "app/project.c"'
assert old in s and 'vendor_lz_probe.h' not in s
f.write_text(s.replace(old,old+'\n#include "app/vendor_lz_probe.h"'))
f=p/'firmware/app.ld';s=f.read_text();old='KEEP(*(.text.crt0))';assert old in s
f.write_text(s.replace(old,old+' KEEP(*(.text.vendor_lz_probe))'))
