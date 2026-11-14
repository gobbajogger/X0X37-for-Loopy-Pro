#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile current ui.c REC feeder and Free Play owner gate into a host stub fixture.

Scoped integration: real production functions, mocked engine/input, no DSP/whole-app.
"""
from pathlib import Path
import sys
root = Path(__file__).resolve().parents[2]
source = (root / 'firmware/src/app/ui.c').read_text()
owner_a = 'static unsigned live_token(unsigned physical)\n{'
owner_b = '\nstatic void ui_live_cancel_voice'
feeder_a = 'static uint16_t rec_outer_prev, rec_outer_suppress, rec_outer_admitted;'
feeder_b = '\n#endif\n\nstatic void input(void)'
input_a = 'static void input(void)\n{'
input_b = '\n/* ================================================================== drawing === */'
def extract(a, b):
    assert source.count(a) == 1, f'exact production source start missing/ambiguous: {a}'
    start = source.index(a)
    assert source.count(b, start) >= 1, f'exact production source end missing: {b}'
    end = source.index(b, start)
    return source[start:end]
def exact_line(prefix):
    lines = [line for line in source.splitlines() if line.startswith(prefix)]
    assert len(lines) == 1, f'exact production dependency missing/ambiguous: {prefix}'
    assert lines[0].endswith('}'), f'production dependency is no longer one line: {prefix}'
    return lines[0]
predicates = '\n'.join(exact_line(prefix) for prefix in (
    'static int part_is_303(', 'static int is_303(',
    'static int bidx(', 'static int kbd_on('))
owner = extract(owner_a, owner_b)
feeder = extract(feeder_a, feeder_b)
input_fn = extract(input_a, input_b)
assert '#include "rec_outer_keymap.h"' in source
assert 'rec_outer_bits(plat_physical_keys())' in feeder
assert 'return bidx()+1;' in owner
assert 'rec_outer_keys();' in input_fn
code = (root / 'tests/host/rec37_owner_integration_test.c').read_text()
assert code.count('/* EXACT_PRODUCTION_FUNCTIONS */') == 1
code = code.replace('/* EXACT_PRODUCTION_FUNCTIONS */', predicates + '\n' + owner + '\n' + feeder + '\n' + input_fn)
out = Path(sys.argv[1])
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(code)
print('production extraction: owner=%d feeder=%d input=%d' % tuple(map(len, (owner, feeder, input_fn))))
