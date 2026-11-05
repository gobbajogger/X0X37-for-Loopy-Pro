#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compare the actual engine LUT helper to its pinned pre-change expression."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parents[2]
BASE = '9231c26285c09356b49b2d9afbcd225377aae5c5'
mode = sys.argv[1] if len(sys.argv) > 1 else 'level'
assert mode in ('level', 'send')
EXPRESSION = '(float)val * (float)val / (100.0f * 100.0f)' if mode == 'level' else '(float)val / 127.0f'
name = 'mix_' + mode + '_from_pot'
fields = ['mix_level'] if mode == 'level' else ['mix_rev', 'mix_dly']
source = (ROOT / 'firmware/src/app/engine.c').read_text()
old = subprocess.check_output(['git', 'show', BASE + ':firmware/src/app/engine.c'], cwd=ROOT, text=True)
for field in fields:
    assert field + '[v] = ' + EXPRESSION + ';' in old
    assert field + '[v] = ' + name + '(val);' in source
helper = re.search(r'static float ' + name + r'\(int val\)\n\{.*?\n\}', source, re.S).group()
program = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
_Static_assert(sizeof(float) == 4, "binary32 required");
HELPER
__attribute__((noinline)) static float reference(int val) { return EXPRESSION; }
static int check(int val) {
    float a = reference(val), b = mix_level_from_pot(val);
    uint32_t ua, ub;
    memcpy(&ua, &a, 4); memcpy(&ub, &b, 4);
    if (ua != ub) { printf("FAIL %d %08x %08x\n", val, ua, ub); return 1; }
    return 0;
}
int main(void) {
    uint32_t seed = 0x672ae19u;
    /* Exhaust the full 7-bit table and nearby non-pot fallback inputs. */
    for (int v = -65536; v <= 65536; ++v) if (check(v)) return 1;
    const int edges[] = {INT_MIN, INT_MIN+1, INT_MAX, INT_MAX-1, -1, 0, 127, 128, 255, 4095};
    for (unsigned k=0; k<sizeof(edges)/sizeof(*edges); ++k) if(check(edges[k])) return 1;
    for (unsigned k=0; k<10000000u; ++k) {
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        int v; memcpy(&v, &seed, sizeof v); if(check(v)) return 1;
    }
    puts("PASS: 128 exhaustive pots, 131073 nearby integers, 10M random integers, int edges; bit-identical");
    return 0;
}
'''.replace('HELPER', helper).replace('EXPRESSION', EXPRESSION).replace('mix_level_from_pot', name)
with tempfile.TemporaryDirectory(prefix='engine-mix-lut-') as tmp:
    p = Path(tmp)
    (p / 'test.c').write_text(program)
    command = [os.environ.get('CC', 'cc'), '-std=c11', '-O2', '-ffp-contract=off', '-fno-fast-math', '-Wall', '-Wextra', '-Werror', str(p/'test.c'), '-o', str(p/'test')]
    print(' '.join(command), flush=True)
    subprocess.run(command, check=True)
    subprocess.run([str(p/'test')], check=True)
