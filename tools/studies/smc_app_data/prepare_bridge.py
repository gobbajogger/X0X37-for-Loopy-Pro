# SPDX-License-Identifier: GPL-3.0-only
"""NON-INSTALLABLE: overlay real copied vendor bridge on prepared UI admission export."""
from pathlib import Path
import shutil
import sys
if len(sys.argv) != 2:
    raise SystemExit('usage: prepare_bridge.py DISPOSABLE_ADMISSION_EXPORT')
root = Path(__file__).resolve().parents[3]
export = Path(sys.argv[1]).resolve()
if (export / '.git').exists():
    raise SystemExit('refuse a Git worktree')
if 'NON-INSTALLABLE: unsealed app-data descriptor' not in (export / 'tools/build.py').read_text():
    raise SystemExit('first prepare relocation and admission export')
for name in ('firmware/src/x0x.c', 'firmware/src/ble_vendor.c',
             'firmware/src/ota.c', 'firmware/hal/x0x_bundle_gate.h',
             'firmware/src/app/editor.c', 'firmware/src/app/plat.h', 'firmware/hal/x0x_smc_readback.h',
             'firmware/hal/x0x_smc_profile.h',
             'firmware/hal/fm1_smc_vendor.h', 'firmware/hal/x0x_ble_segment.h',
             'firmware/hal/x0x_smc_host_messages.h', 'firmware/src/ble/smc_host_ingress.h',
             'firmware/src/dsp/drum808.c', 'firmware/src/app/smc_mixer.c', 'firmware/src/app/x0x.h', 'firmware/src/app/ui.c'):
    shutil.copyfile(root / name, export / name)
print('NON-INSTALLABLE: real bridge sizing; default app ceiling is NOT segmented admission')
ui = export / 'firmware/src/app/ui.c'
s = ui.read_text()
old = 'CORNER_OFF[RMAX + 1] ='
if s.count(old) != 1:
    raise SystemExit('expected original corner table for relocation')
ui.write_text(s.replace(old, 'CORNER_OFF[RMAX + 1] __attribute__((section(".app_ui.CORNER_OFF"))) ='))
build = export / 'tools/build.py'
s = build.read_text()
needle = 'tc("pi32v2/bin/ld", "--gc-sections", "-e", "_start", "-T", ld, *objs, "-o", elf)'
if needle in s:
    s = s.replace(needle, 'tc("pi32v2/bin/ld", "--gc-sections", "-Map", OUT / "x0x.map", "-e", "_start", "-T", ld, *objs, "-o", elf)')
    build.write_text(s)
elif '"-Map", OUT / "x0x.map"' not in s:
    raise SystemExit('unrecognized app link command; cannot retain map')

# An explicit compile interlock prevents ordinary profile activation. The only
# supplied opt-in route requires this already packaging-disabled study export.
s = build.read_text()
if '"-DX0X_SMC_VENDOR_STUDY_ONLY=1"' not in s:
    needle = '"-DX0X_SMC_VENDOR_TEST=1"'
    if needle not in s: raise SystemExit('missing explicit vendor study profile')
    build.write_text(s.replace(needle, needle + ', "-DX0X_SMC_VENDOR_STUDY_ONLY=1"'))
