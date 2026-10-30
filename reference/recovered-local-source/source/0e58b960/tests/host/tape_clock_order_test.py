#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Source-extracted getter: audio IRQ may publish after the clock is sampled."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parents[2]
s=(root/'firmware/src/app/engine.c').read_text()
getter=re.search(r'uint8_t engine_tape_active\(void\)\n\{.*?\n\}',s,re.S).group()
code="""#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static volatile uint8_t tape_signal_seen=1;static volatile uint32_t tape_signal_at=100;
static uint32_t now=100;
static uint32_t plat_ms(void){uint32_t sampled=now;tape_signal_at=++now;return sampled;}
"""+getter+"""
int main(void){int active=engine_tape_active();assert(now==101&&tape_signal_at==101&&tape_signal_seen);assert(active);puts("Actual getter clock/IRQ ordering PASS");return 0;}
"""
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'probe.c').write_text(code)
 subprocess.run(['cc','-O2',str(p/'probe.c'),'-o',str(p/'probe')],check=True)
 subprocess.run([str(p/'probe')],check=True)
