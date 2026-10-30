#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile the actual read-only snapshot writer; inject ISR-like changes and decode its bytes.
No MIDI backend or hardware is used. Optional CFLAGS adds host sanitizers.
"""
import contextlib
import importlib.util
import io
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile
import types

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import inlog_fmt as fmt

main = (ROOT / 'firmware/src/app/main_fm1.c').read_text()
hal = (ROOT / 'firmware/hal/smk_input.h').read_text()
editor = (ROOT / 'firmware/src/app/editor.c').read_text()

def function(source, name):
    match = re.search(r'^static (?:void|inline void) ' + name + r'\([^\n]*\).*?\n\{', source, re.M)
    assert match, name
    start = match.start()
    i, depth = match.end(), 1
    while depth:
        depth += (source[i] == '{') - (source[i] == '}')
        i += 1
    return source[start:i]

state = re.search(r'static volatile struct \{.*?\} fm1_in;', hal, re.S).group()
copy = function((ROOT / 'firmware/src/app/plat_fm1.c').read_text(), 'smk_pad_snapshot_copy')
writer = copy.replace('{\n', '{\n    (void)s; /* stores instrumented below */\n', 1) + '\n' + function(main, 'smk_pad_inputs_reply')
assert 'cli' not in writer and 'fm1__lock' not in writer
assert 'arg == 6u' in function(main, 'smk_inlog_reply')
assert main.count('smk_pad_inputs_reply();') == 1
# Real writer ordering: the input state is written by the CPU0 timer path.
tick = function(hal, 'fm1_input_tick')
assert tick.index('fm1_in.adc_ok++') < tick.index('smk__analog(')
assert tick.index('fm1_in.adc_miss++') < tick.index('fm1__frame()')
assert 'static int ed_service(void)' in editor
prefix = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define FM1_NCOL 16u
#define FM1_NBTN 18u
#define FM1_NKEY 37u
#define FM1_NENC 8u
#define SMK_NPAD 16u
#define SMK_PAD_ON 24u
#define SMK_PAD_OFF 12u
#define SMK_PAD_PEAKN 3u
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
static uint8_t ed_out[732];
#define ED_SMP_BUF_SIZE 512u
static uint8_t ed_smp_buf[ED_SMP_BUF_SIZE];
static uint32_t ed_n;
static uint32_t fm1_ms = 0xffffffffu;
static uint32_t smk_diag_ack;static uint16_t smk_diag_seen;
static uint8_t smk_pad_grid = 1, smk_pad_piano = 1, smk_pad_note[16];
static uint16_t smk_trim_pad[16][5];
static int inject_at = -1, repeat, kind, changes, inject_copy = 1;
'''
hook = r'''
static void inject(unsigned at, int copy) {
    if ((int)at != inject_at || inject_copy != copy) return;
    if (!repeat) inject_at = -1;
    changes++;
    if (kind == 0) fm1_in.adc_ok++;
    else if (kind == 1) fm1_in.adc_miss++;
    else fm1_in.frames++;
    for (unsigned i = 0; i < 16; i++) fm1_in.pad_raw[i] ^= 0x155u;
}
static void capture_b(unsigned at, uint8_t value) {
    ed_smp_buf[at] = value;
    inject(at, 1);
}
'''
# Inject a complete ISR-like update after each actual scratch byte store.
writer = re.sub(r's\[([^\]]+)\] = ([^;]+);', r'capture_b(\1, \2);', writer)
byte_writer = function(editor, 'ed_b').replace('{\n', '{\n    inject(ed_n, 0);\n', 1)
exercise = r'''
static uint32_t word(unsigned at, unsigned n) {
    uint32_t v = 0;
    for (unsigned j = 0; j < n; j++) v |= (uint32_t)ed_out[at+j] << (7*j);
    return v;
}
static void init(void) {
    memset((void *)&fm1_in, 0, sizeof fm1_in);
    fm1_in.adc_ok = fm1_in.adc_miss = fm1_in.frames = 0xffffffffu;
    fm1_in.pnotes = 0x04000001u;
    for (unsigned i = 0; i < 16; i++) {
        fm1_in.pad_raw[i] = i == 15 ? 1023 : i * 61;
        fm1_in.pad_base[i] = i == 15 ? 65535 : 50 + i;
        fm1_in.pad_peak[i] = i == 0 ? 0 : 1023-i;
        fm1_in.pad_st[i] = i & 1 ? 0x80 : i % 3;
        fm1_in.pad_vel[i] = i == 15 ? 127 : i;
        fm1_in.pad_press[i] = 127-i;
        smk_pad_note[i] = i == 15 ? 27 : i;
        for (unsigned j = 0; j < 5; j++) smk_trim_pad[i][j] = i*100 + j;
    }
    smk_trim_pad[15][4] = 65535;
    for (unsigned i = 0; i < 4; i++) fm1_in.fad_raw[i] = 100+i;
    fm1_in.wheel[0] = 0; fm1_in.wheel[1] = 1023;
    fm1_in.pedal = 1000; fm1_in.batt = 600;
    memset(ed_out, 0xa5, sizeof ed_out);
    memset(ed_smp_buf, 0xa5, sizeof ed_smp_buf);
    ed_n = 5; changes = 0;
}
static void check(int serial_irq) {
    assert(ed_n == 526); /* 521-byte payload; F7 adds one byte */
    for (unsigned i = 5; i < ed_n; i++) assert(ed_out[i] < 128);
    for (unsigned i = ed_n; i < sizeof ed_out; i++) assert(ed_out[i] == 0xa5);
    for (unsigned i = 176; i < sizeof ed_smp_buf; i++) assert(ed_smp_buf[i] == 0xa5);
    for (unsigned i = 0; i < 16; i++)
        assert(word(5+57+i*29,3) == (fm1_in.pad_raw[i] ^ (serial_irq ? 0x155u : 0)));
    assert(word(5+18,5) == fm1_in.adc_ok - (serial_irq && kind == 0));
    assert(word(5+23,5) == fm1_in.adc_miss - (serial_irq && kind == 1));
    assert(word(5+13,5) == fm1_in.frames - (serial_irq && kind == 2));
}
int main(void) {
    unsigned char before[sizeof fm1_in];
    init(); memcpy(before, (const void *)&fm1_in, sizeof fm1_in);
    smk_pad_inputs_reply(); check(0);
    assert(!memcmp(before, (const void *)&fm1_in, sizeof fm1_in));
    for (unsigned i = 5; i < ed_n; i++) printf("%u%c",ed_out[i],i+1==ed_n?'\n':' ');
    for (kind = 0; kind < 3; kind++) {
        for (unsigned p = 0; p < 176; p++) {
            init(); inject_copy = 1; inject_at = (int)p;
            smk_pad_inputs_reply();
            assert(changes == 1); check(0); /* copy retries cover counter wrap to zero */
        }
        for (unsigned p = 5; p < 526; p++) {
            init(); inject_copy = 0; inject_at = (int)p;
            smk_pad_inputs_reply();
            assert(changes == 1); check(1); /* serialization retains the pre-IRQ snapshot */
        }
        init(); repeat = 1; inject_copy = 1; inject_at = 30;
        smk_pad_inputs_reply();
        assert(changes == 3 && ed_n == 8);
        assert(ed_out[5] == 6 && ed_out[6] == 1 && ed_out[7] == 1);
        repeat = 0; inject_at = -1;
    }
    puts("PASS actual writer: 528 copy collisions + 1563 serialization IRQs, wrap, bounded BUSY, scratch bounds, no state writes");
}
'''
with tempfile.TemporaryDirectory(prefix='smk-pad-inputs-') as td:
    c, exe = Path(td)/'test.c', Path(td)/'test'
    c.write_text(prefix + state + hook + byte_writer + '\n' + function(main, 'smk_inlog_w') + '\n' + writer + exercise)
    cmd = [os.environ.get('CC','cc'), '-std=c99','-O2','-Wall','-Wextra','-Werror']
    cmd += shlex.split(os.environ.get('CFLAGS','')) + [str(c), '-o', str(exe)]
    subprocess.run(cmd, check=True)
    lines = subprocess.check_output([str(exe)], text=True).splitlines()
print(lines[-1])
r = [int(x) for x in lines[0].split()]
d = fmt.parse_pad_inputs(r)
assert d and d['status'] == 'snapshot' and len(d['pads']) == 16
assert all(d[k] == 0xffffffff for k in ('now_ms','frames','adc_ok','adc_miss'))
assert d['mux_c'] == [100,101,102,103,0,1023,1000,600]
assert d['pads'][0]['raw'] == 0 and d['pads'][15]['raw'] == 1023
assert d['pads'][15]['base'] == 65535 and d['pads'][15]['curve'][-1] == 65535
assert all(p['pad'] == i and p['state'] == (128 if i&1 else i%3) for i,p in enumerate(d['pads']))
assert d['pnotes'] == 0x04000001 and d['grid'] and d['piano']
for n in range(len(r)):
    assert fmt.parse_pad_inputs(r[:n]) is None
assert fmt.parse_pad_inputs(r+[0]) is None
for index,value in ((0,5),(1,2),(2,2),(3,15),(4,4),(8,128),(12,16),(59,4),(67,2)):
    bad = r.copy(); bad[index] = value
    assert fmt.parse_pad_inputs(bad) is None, (index,value)
for count in range(65):
    assert fmt.parse_pad_inputs([count] + [0]*(85+10*count)) is None
assert fmt.parse_pad_inputs([6,1,1]) == {'version':1,'status':'busy'}
assert fmt.parse_pad_inputs([6,1,1,0]) is None
assert 'no measurements' in fmt.pad_inputs_text({'status':'busy'})[0]
assert 'no sensor conclusion' in fmt.pad_inputs_text(None)[0]
assert len(fmt.pad_inputs_text(d)) == 21
print('PASS strict v1 decode: all 16 pads, extremes, malformed/legacy/BUSY replies')
# Load the existing USB tool with a fake backend. The flag must not reach reset paths,
# even when other existing flags are supplied. No real ports are opened.
sys.modules['mido'] = types.ModuleType('mido')
spec = importlib.util.spec_from_file_location('pad_inputs_cli_test', ROOT/'tools/ble_autotest.py')
cli = importlib.util.module_from_spec(spec); spec.loader.exec_module(cli)
cli.ports = lambda: ('fake-in','fake-out')
for reply,expected in ((r,0),([6,1,1],1),(None,1)):
    calls = []
    def ask(cmd,args=()):
        calls.append((cmd,args)); return reply
    cli.ask = ask
    old = sys.argv
    sys.argv = ['ble_autotest.py','--pad-inputs','--diag-only','--padcal-export','must-not-write']
    try:
        with contextlib.redirect_stdout(io.StringIO()):
            assert cli.main() == expected
    finally:
        sys.argv = old
    assert calls == [(38,[6])]
print('PASS --pad-inputs sends only INLOG 6; unsupported/BUSY fail closed, no reset or calibration writes')
