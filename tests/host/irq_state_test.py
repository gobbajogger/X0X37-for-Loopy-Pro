#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Run the production guards with only pi32v2 instructions modelled on host."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"static inline [^{]+\b" + name + r"\([^)]*\)\s*\{", source)
    assert match, name
    start, pos, depth = match.start(), match.end(), 1
    while depth:
        depth += (source[pos] == "{") - (source[pos] == "}")
        pos += 1
    return source[start:pos]


flash = (ROOT / 'firmware/hal/fm1_flash.h').read_text()
smk = (ROOT / 'firmware/hal/smk_input.h').read_text()
code = '\n'.join(function(flash, n) for n in ('irq_save', 'irq_restore'))
for name in ('fm1__lock', 'fm1__unlock'):
    alias = re.search(r'^#define ' + name + r'\b[^\n]*', smk, re.M)
    code += '\n' + (alias.group() if alias else function(smk, name))
# Model instructions, not the guard's C control flow. The original unconditional
# sti therefore enables the model too early and fails the nested assertion.
code = code.replace('__asm__ volatile("cli" ::: "memory");', 'state &= ~(1u << 8);')
code = code.replace('__asm__ volatile("csync\\n\\tsti" ::: "memory");', 'state |= 1u << 8;')
code = code.replace('__asm__ volatile("csync" ::: "memory");', '')
code = code.replace('__asm__ volatile("%0 = icfg" : "=r"(flags) :: "memory");', 'flags = state;')
code = code.replace('__asm__ volatile("icfg = %0" :: "r"(disabled) : "memory");', 'state = disabled;')
code = code.replace('__asm__ volatile("icfg = %0" :: "r"(flags) : "memory");', 'state = flags;')
assert '__asm__' not in code, 'unmodelled hardware instruction'
test = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static uint32_t state;
'''+code+r'''
int main(void)
{
    const uint32_t initial[] = {0u, 0x100u, 0x200u, 0x300u, 0xa5a5b7ffu};
    for (unsigned i = 0; i < sizeof initial / sizeof initial[0]; i++) {
        state = initial[i];
        uint32_t outer = irq_save();
        assert(!(state & 0x100u));
        uint32_t disabled = state;
        uint32_t inner = fm1__lock();
        assert(state == disabled);
        fm1__unlock(inner);
        assert(state == disabled);
        irq_restore(outer);
        assert(state == initial[i]);
        outer = fm1__lock();
        inner = irq_save();
        irq_restore(inner);
        assert(!(state & 0x100u));
        fm1__unlock(outer);
        assert(state == initial[i]);
    }
    puts("PASS: production IRQ/input guards preserve nested and disabled ICFG states");
}
'''
with tempfile.TemporaryDirectory(prefix='qb-irq-') as temp:
    path = Path(temp)
    (path / 'test.c').write_text(test)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-Wall', '-Wextra',
                    '-Wno-unused-function', str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
