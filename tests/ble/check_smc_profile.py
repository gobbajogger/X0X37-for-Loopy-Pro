#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile the actual selector guards without building or packaging firmware."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
base=['cc','-std=c11','-Werror','-x','c','-fsyntax-only','-I'+str(root/'firmware/hal'),'-']
src='#include "x0x_smc_profile.h"\n'
cases=[([],True),(['X0X_SMC_VENDOR_TEST=1','BOARD_SMK37=1','X0X_BLE=1'],False),
 (['X0X_SMC_VENDOR_TEST=1','BOARD_SMK37=1','X0X_BLE=1','X0X_SMC_VENDOR_STUDY_ONLY=1'],True)]
dev=['X0X_SMC_VENDOR_TEST=1','BOARD_SMK37=1','X0X_BLE=1','X0X_SMC_VENDOR_DEV_PACKAGE=1','X0X_DEV_PADCAL=1','X0X_SMC_VENDOR_MODEL_ELITE=1','SMK_MODEL="SMK-37 Elite"']
cases.append((dev,True))
cases += [([d for d in dev if not d.startswith('X0X_BLE=')],False),
          ([d if not d.startswith('X0X_SMC_VENDOR_DEV_PACKAGE=') else 'X0X_SMC_VENDOR_DEV_PACKAGE=2' for d in dev]+['X0X_SMC_VENDOR_STUDY_ONLY=-1'],False)]
for omitted in ('X0X_DEV_PADCAL','X0X_SMC_VENDOR_MODEL_ELITE','BOARD_SMK37'):
    cases.append(([d for d in dev if not d.startswith(omitted+'=')],False))
cases += [(dev+['X0X_SMC_VENDOR_STUDY_ONLY=1'],False),
          ([d if not d.startswith('SMK_MODEL=') else 'SMK_MODEL="SMK-37 Pro"' for d in dev],False)]
for defs,want in cases:
    p=subprocess.run(base[:-1]+['-D'+d for d in defs]+['-'],input=src,text=True,capture_output=True)
    if (p.returncode==0)!=want:raise RuntimeError(str(defs)+'\n'+p.stderr)
print('SMC selectors: default/study/Elite DEV, missing guards, mixed selector and Pro refusal PASS')
