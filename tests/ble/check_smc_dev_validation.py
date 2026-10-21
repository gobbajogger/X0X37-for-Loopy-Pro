#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Selection/env rejection tests; does not build or grant DEV evidence."""
import json
from pathlib import Path
import subprocess,sys,tempfile,types
root=Path(__file__).resolve().parents[2];sys.path.insert(0,str(root/'tools'))
import dev_validate as dv
clean=dv.development_env({'X0X_SMC_VENDOR_TEST':'1','X0X_SMC_VENDOR_STUDY_ONLY':'1','X0X_DEV_PADCAL':'0'},'elite')
if any(k.startswith('X0X_SMC_') for k in clean) or clean['X0X_DEV_PADCAL']!='1':raise RuntimeError('ambient SMC selector leaked')
# Selection fails before reading any package or calling the bundle validator.
sys.modules['smc_vendor_bundle']=types.SimpleNamespace(validate_bundle=lambda *a,**kw: (_ for _ in ()).throw(RuntimeError('unexpected package access')))
import smc_vendor_dev_validate as sv
with tempfile.TemporaryDirectory() as d:
    p=Path(d)/'record.json'
    base={'schema':3,'result':'DEV CHECKED','profile':'smc-vendor-central-dev','model':'elite','config':'dev','hardware_tested':False}
    for field,value in [('schema',2),('profile','default'),('model','pro'),('config','rel'),('hardware_tested',True)]:
        r=dict(base);r[field]=value;p.write_text(json.dumps(r))
        try:sv.verify(root,'HEAD',p,'absent.fwsc')
        except ValueError:pass
        else:raise RuntimeError('invalid profile record accepted')
    p.write_text(json.dumps(base))
    try:dv.verify(root,'HEAD',p,'absent.fwsc')
    except ValueError:pass
    else:raise RuntimeError('generic verification accepted segmented record')
    try:dv.verify(root,'HEAD',p,'absent.fwsc','ctl.img',True)
    except ValueError:pass
    else:raise RuntimeError('ABI2 argument accepted in segmented route')
print('SMC DEV validation: ambient selectors removed, explicit schema/profile/model/config/controller refusal PASS')
