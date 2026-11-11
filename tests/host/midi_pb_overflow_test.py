#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile production input/output/BLE queues for the archived PB flood fix."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
mode = sys.argv[1]
assert mode in ('in', 'out', 'ble')
usb = (ROOT / 'firmware/src/usb.c').read_text()
ble = (ROOT / 'firmware/src/ble.c').read_text()


def function(source, name):
    m = re.search(r'^[^\n]*\b' + name + r'\([^\n]*\)[^\n{]*\s*\{', source, re.M)
    assert m, name
    end, depth = m.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[m.start():end]


hardware = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define RING_PUBLISH() ((void)0)
#define FM1_IRQ_TIMER5 63u
static uint32_t enabled = 1;
static unsigned held, saves, restores;
static uint32_t fm1_irq_mask_save(unsigned n)
{ assert(n == FM1_IRQ_TIMER5 && !held); held = 1; saves++; uint32_t v = enabled; enabled = 0; return v; }
static void fm1_irq_mask_restore(unsigned n, uint32_t v)
{ assert(n == FM1_IRQ_TIMER5 && held && !enabled); enabled = v; held = 0; restores++; }
'''
if mode == 'ble':
    size = re.search(r'^#define BLE_OQ (\d+)u', ble, re.M).group(1)
    declarations = f'#define BLE_OQ {size}u\n' + r'''
static uint32_t ble_oq[BLE_OQ];
static struct { volatile uint32_t ow, orr; union { int h; } host; } ble_r;
static int ble_central_role, ble_host_on = 1, ble_want = 1;
static int smc_ble_role_get(void) { return 0; }
static int bleh_notifying(void *h) { (void)h; return 1; }
'''
    start = ble.index('/* CIN 8 / CIN 9 vel0')
    production = ble[start:ble.index('\nstatic void ble_fail(', start)]
    adapter = r'''
#define Q ble_oq
#define W ble_r.ow
#define R ble_r.orr
#define CAP BLE_OQ
static int send(uint32_t p) { ble_out(p); return 1; }
'''
else:
    size = re.search(r'^#define MQ (\d+)u', usb, re.M).group(1)
    declarations = f'#define MQ {size}u\n'
    start = usb.index('/* CIN 8, or CIN 9 with velocity 0')
    end = usb.index('/* Enqueue into midi_in_q.', start)
    production = usb[start:end]
    if mode == 'in':
        declarations += 'static uint32_t midi_in_q[MQ];\nstatic uint8_t mi_fresh[MQ];\nstatic volatile uint32_t mi_w, mi_r;\n'
        production += function(usb, 'midi_in_put')
        adapter = '#define Q midi_in_q\n#define W mi_w\n#define R mi_r\n#define CAP MQ\n#define HAS_RESULT 1\n#define SUPPORT_LINK_END 1\nstatic int send(uint32_t p) { return midi_in_put(p); }\n'
    else:
        declarations += 'static uint32_t midi_in_q[MQ], midi_out_q[MQ];\nstatic uint8_t mi_fresh[MQ];\nstatic volatile uint32_t mo_w, mo_r;\nstatic struct { int config; } usb = {1};\n'
        production += function(usb, 'midi_out_event')
        adapter = '#define Q midi_out_q\n#define W mo_w\n#define R mo_r\n#define CAP MQ\n#define SUPPORT_LINK_END 1\nstatic int send(uint32_t p) { midi_out_event(p); return 1; }\n'

with tempfile.TemporaryDirectory(prefix='x0x-pb-' + mode + '-') as directory:
    path = Path(directory)
    (path / 'production.h').write_text(hardware + declarations + production + '\n' + adapter)
    command = shlex.split(os.environ.get('CC', 'cc')) + ['-std=c11', '-O2', '-Wall', '-Wextra', '-Werror']
    command += shlex.split(os.environ.get('CFLAGS', ''))
    subprocess.run(command + ['-I', str(path), str(ROOT / 'tests/host/midi_pb_overflow_test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
