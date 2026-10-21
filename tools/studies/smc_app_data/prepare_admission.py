# SPDX-License-Identifier: GPL-3.0-only
# NON-INSTALLABLE: overlay the opt-in helper on a prepared relocation export.
from pathlib import Path
import shutil
import sys
if len(sys.argv) != 2:
    raise SystemExit('usage: prepare_admission.py DISPOSABLE_PREPARED_EXPORT')
root = Path(__file__).resolve().parents[3]
export = Path(sys.argv[1]).resolve()
if (export / '.git').exists():
    raise SystemExit('refuse a Git worktree')
build = export / 'tools/build.py'
s = build.read_text()
if 'NON-INSTALLABLE: app relocation sizing only' not in s:
    raise SystemExit('first run prepare.py on a baseline archive export')
for rel in ('firmware/hal/x0x_app_ui_data.h', 'firmware/hal/fm1_flash_map.h',
            'firmware/src/app/main_fm1.c', 'firmware/src/storage.c'):
    shutil.copyfile(root / rel, export / rel)
s = s.replace('CFLAGS = ["-Os",', 'CFLAGS = ["-DX0X_SMC_VENDOR_TEST=1", "-DX0X_SMC_VENDOR_STUDY_ONLY=1", "-Os",')
s = s.replace('NON-INSTALLABLE: app relocation sizing only; no data admission or package',
              'NON-INSTALLABLE: unsealed app-data descriptor; no update transaction or package')
build.write_text(s)
