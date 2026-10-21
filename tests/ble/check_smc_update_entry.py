#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Source wiring plus native OTA gate transitions; no hardware."""
from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[2]
s=(r/'firmware/src/app/main_fm1.c').read_text()
a=s.index('    bp_update_boot(&smc_update_gate, proj.set.bt & 1u);')
if not s.rfind('project_load()',0,a)<a<s.index('    ble_prepare(proj.set.bt & 1u);'):
    raise RuntimeError('gate must follow loaded settings and precede controller prepare')
s=(r/'firmware/src/x0x.c').read_text().split('static void ota_commit(const uint8_t *parm)')[1].split('\n}')[0]
if not s.index('#if X0X_SMC_VENDOR_TEST')<s.index('fm1_audio_stop();')<s.index('#endif')<s.index('fm1_enter_update(parm);'):
    raise RuntimeError('TEST audio DMA stop must precede reset/SPL')
with tempfile.TemporaryDirectory() as d:
    out=Path(d)/'gate'
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Wno-sign-compare','-Wno-maybe-uninitialized',str(r/'tests/ble/vendor_update_gate_test.c'),'-o',str(out)],check=True)
    subprocess.run([str(out)],check=True)
print('TEST cold-OFF entry wiring and pre-reset audio stop order PASS')
