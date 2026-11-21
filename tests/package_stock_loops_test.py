#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Synthetic package proof; fixtures are not executable firmware or device validation."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

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

starting = (ROOT / 'assets/starting-breaks/starting-break-pack.bin').read_bytes()
assert len(starting) == 109394 and hashlib.sha256(starting).hexdigest() == ps.bp.STARTING_SHA256
# A linked application's declared extent includes erased-pattern literals and padding.
linked_with_ff = b'code' + b'\xff' * 93
linked_end = (len(linked_with_ff) + 15) & ~15
exact_limit = linked_end + len(starting) + ps.bp.SMK_XIP_RESERVE
assert ps.bp.embed_starting_breaks(linked_with_ff, exact_limit) == linked_with_ff.ljust(linked_end, b'\xff') + starting
try:
    ps.bp.embed_starting_breaks(linked_with_ff, exact_limit - 1)
except ValueError as exc:
    assert str(exc) == f'loop pack needs {len(starting)} bytes, {len(starting) - 1} free'
else:
    raise AssertionError('native build overwrote the declared application extent or the reserve')
rate, loops = ps.bp.parse_pack(starting)
assert rate == 22050
assert [loop['name'] for loop in loops] == ['BR1 1', 'BR1 2', 'BR1 3', 'BR2 1', 'BR2 2']
assert [loop['nsamples'] for loop in loops] == [34800, 41788, 42862, 46184, 52913]
expected = b''.join(f"{loop['name']}\t{rate}\t{loop['nsamples']}\t1\t{len(loop['adpcm'])}\n".encode()
                    + loop['adpcm'] for loop in loops)
with tempfile.TemporaryDirectory() as tmp:
    td = Path(tmp)
    reader = td / 'reader'
    subprocess.run(['cc', '-O2', '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / 'firmware/src'),
                    str(ROOT / 'tests/brk_pack_test.c'), '-o', str(reader)], check=True)
    for model in ('elite', 'pro'):
        limit = ps.ctl.CODE_MAX
        last_start = (limit - ps.bp.SMK_XIP_RESERVE - len(starting)) & ~15
        # Fill through the last usable alignment, using the real format and CLI.
        plain = make_package(model)
        _, _, state = ps.pn._package(plain, model)
        used = len(state['area'][0x120:0x120 + limit].rstrip(b'\xff'))
        padding = b'\xa5' * (last_start - used)
        linked = state['area'][0x120:0x120 + used] + padding
        embedded = ps.bp.embed_starting_breaks(linked, limit)
        assert embedded[:len(linked)] == linked and embedded[last_start:] == starting
        raw = ps.ctl.add(make_package(model, extra=padding), controller)
        source = td / (model + '.fwsc')
        output = td / (model + '-packed.fwsc')
        receipt_path = td / (model + '.json')
        source.write_bytes(raw)
        cli = [sys.executable, str(ROOT / 'tools/package_stock_loops.py'), str(source),
               '--starting-breaks', '--model', model, '--package-sha256', hashlib.sha256(raw).hexdigest(),
               '--controller-sha256', cpin, '--controller-size', str(len(controller)),
               '--output', str(output), '--receipt', str(receipt_path)]
        subprocess.run(cli, check=True, capture_output=True)
        receipt = json.loads(receipt_path.read_text())
        assert receipt['bank_offset'] == last_start and receipt['bank_sha256'] == ps.bp.STARTING_SHA256
        assert receipt['xip_reserve'] <= receipt['remaining_code_tail'] < receipt['xip_reserve'] + 16
        _, _, saved = ps.pn._package(output.read_bytes(), model)
        app = saved['area'][0x120:0x120 + limit]
        image = td / (model + '-app.bin')
        image.write_bytes(app)
        assert subprocess.check_output([str(reader), str(image), '--extract']) == expected
        assert app == embedded.ljust(limit, b'\xff')
        # An extra byte of used application space moves the bank to the next alignment.
        too_large = ps.ctl.add(make_package(model, extra=padding + b'\xa5'), controller)
        try:
            ps.compose(too_large, model, hashlib.sha256(too_large).hexdigest(), cpin, len(controller), starting)
        except ValueError as exc:
            remaining = limit - ps.bp.SMK_XIP_RESERVE - (last_start + 16)
            assert str(exc) == f'loop pack needs {len(starting)} bytes, {remaining} free'
        else:
            raise AssertionError('owner pack admitted beyond the erased reserve')
        print('PASS', model, 'five original zones through CLI and native reader; stock reserve/alignment boundary')
