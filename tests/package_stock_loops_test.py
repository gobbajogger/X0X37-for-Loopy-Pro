#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Synthetic package proof; fixtures are not executable firmware or device validation."""
import hashlib
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from nightly_package_test import make_package, make_controller
import package_stock_loops as ps

bank = (ROOT / 'assets/factory-breaks/factory-break-pack.bin').read_bytes()
controller = make_controller()
cpin = hashlib.sha256(controller).hexdigest()
for model in ('elite', 'pro'):
    raw, _ = ps.pn.compose(make_package(model), controller, model, cpin, len(controller))
    args = dict(raw=raw, model=model, package_sha256=hashlib.sha256(raw).hexdigest(),
                controller_sha256=cpin, controller_size=len(controller), bank=bank)
    final, receipt = ps.compose(**args)
    assert receipt['remaining_code_tail'] >= 0 and receipt['bank_bytes'] == 131152
    assert receipt['compiled_app_preserved'] and receipt['controller_preserved']
    repeated, _ = ps.compose(**{**args, 'raw': final, 'package_sha256': hashlib.sha256(final).hexdigest()})
    assert repeated == final
    assert ps.pn.pm.slotmap_of(ps.ctl.logical_of(raw)[1]) == ps.pn.pm.slotmap_of(ps.ctl.logical_of(final)[1])
    for delta in ({'package_sha256':'0'*64}, {'controller_sha256':'0'*64},
                  {'model':'pro' if model == 'elite' else 'elite'}, {'bank':bank[:-1]}):
        try: ps.compose(**{**args, **delta})
        except (ValueError, ps.ctl.PkgError): pass
        else: raise AssertionError(delta)
    print('PASS', model, 'synthetic package parity, repeat composition, unchanged slot map, negative pins/model/bank')
