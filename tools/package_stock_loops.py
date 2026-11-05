#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Add the designated stock bank to a checked package without recompiling firmware.

Input package and controller pins are mandatory. This is local composition only:
no catalog update, publication, device access or hardware-validation claim.
"""
import argparse
import hashlib
import json
from pathlib import Path

import add_ctl as ctl
import break_pack as bp
import package_nightly as pn

STOCK_SHA256 = 'cbfe682a9c69b51dac4ee96db7ab4605c8ecdb2839230efcc5445aa0a8fe199e'

def compose(raw, model, package_sha256, controller_sha256, controller_size, bank, ledger=None):
    if hashlib.sha256(raw).hexdigest() != package_sha256:
        raise ValueError('input package pin mismatch')
    if hashlib.sha256(bank).hexdigest() != STOCK_SHA256:
        raise ValueError('designated stock bank pin mismatch')
    before = pn.inspect(raw, model, controller_sha256, controller_size, ledger=ledger)
    product, logical, state = pn._package(raw, model, ledger=ledger)
    original = bytes(state['area'])
    app = original[0x120:0x120 + ctl.CODE_MAX]
    placed = bp.place_pack(app, bank, ctl.CODE_MAX)
    positions = [i for i in range(0, ctl.CODE_MAX - 20, 16) if bp._valid_at(placed, i, ctl.CODE_MAX)]
    if len(positions) != 1:
        raise ValueError('expected exactly one valid stock bank')
    offset = positions[0]
    if placed[:offset] != app[:offset]:
        raise ValueError('stock placement changed compiled application bytes')
    state['area'][0x120:0x120 + ctl.CODE_MAX] = placed
    final = ctl.fwsc_of(ctl._close(state), product)
    after = pn.inspect(final, model, controller_sha256, controller_size, ledger=ledger)
    _, final_logical, checked = pn._package(final, model, ledger=ledger)
    allowed = set(range(0x120 + offset, 0x120 + ctl.CODE_MAX)) | {0, 1, 2, 3, 32, 33, 34, 35}
    if len(final) != len(raw) or len(checked['area']) != len(original):
        raise ValueError('package size changed')
    if any(a != b and i not in allowed for i, (a, b) in enumerate(zip(original, checked['area']))):
        raise ValueError('bytes outside bank tail/checksums changed')
    if ctl.controller_of(final_logical) != ctl.controller_of(logical):
        raise ValueError('controller bytes changed')
    return final, {'input': before, 'output': after, 'bank_sha256': STOCK_SHA256,
                   'bank_bytes': len(bank), 'bank_offset': offset,
                   'remaining_code_tail': ctl.CODE_MAX - offset - len(bank),
                   'compiled_app_preserved': True, 'controller_preserved': True,
                   'other_package_regions_preserved': True,
                   'state': 'LOCAL_COMPOSED_CHECKED', 'hardware': 'unrun', 'published': False}

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('package', type=Path)
    p.add_argument('--model', choices=['elite', 'pro'], required=True)
    p.add_argument('--package-sha256', required=True)
    p.add_argument('--controller-sha256', required=True)
    p.add_argument('--controller-size', type=int, required=True)
    p.add_argument('--ledger', type=Path)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--receipt', type=Path, required=True)
    a = p.parse_args()
    if a.output.exists() or a.receipt.exists():
        p.error('never overwrite package or receipt')
    bank = Path(__file__).resolve().parents[1] / 'assets/factory-breaks/factory-break-pack.bin'
    result, receipt = compose(a.package.read_bytes(), a.model, a.package_sha256,
                              a.controller_sha256, a.controller_size, bank.read_bytes(), a.ledger)
    ctl.write_package(a.output, result)
    a.receipt.write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))

if __name__ == '__main__': main()
