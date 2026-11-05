#!/usr/bin/env python3
"""Source-selected native regression: reject non-7-bit SMC User-mode MIDI data.

Extracts actual function bodies from the checkout at runtime. Host mocks model
queue admission only; no compiled firmware, device or physical BLE testing.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(os.environ.get('SMC_SOURCE', ROOT / 'firmware/src/app/smc_mixer.c'))
PROTOCOL = Path(os.environ.get('SMC_PROTOCOL', ROOT / 'firmware/src/app/smc_protocol.h'))


def function(text, name):
    m = re.search(r'^[^\n]*\b' + re.escape(name) + r'\([^\n]*\)\s*\{', text, re.M)
    if not m:
        raise ValueError(f'Missing production function: {name}')
    depth, end = 1, m.end()
    while depth:
        if end >= len(text):
            raise ValueError(f'Unterminated production function: {name}')
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[m.start():end]


HOST = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define PROJ_BT_SMC_USER 32u
#define SMC_Q 32u
static volatile uint8_t smc_host_w, smc_host_r;
static struct { volatile uint32_t release_lost[4]; } smc_inputs[2];
#define smc_host_solo_release_lost smc_inputs[0].release_lost[1]
#define smc_pause_release_lost (smc_inputs[0].release_lost + 2)
static uint8_t smc_host_q[SMC_Q][3];
static struct { struct { uint8_t bt; } set; } proj;
'''
TEST = r'''
static uint32_t packet(uint8_t st, uint8_t a, uint8_t b)
{ return 9u | ((uint32_t)st << 8) | ((uint32_t)a << 16) | ((uint32_t)b << 24); }
static unsigned tested, failed;
static void check(const char *name, uint8_t st, uint8_t a, uint8_t b, int enabled, int expected)
{
    smc_host_r = smc_host_w = 0;
    memset(smc_host_q, 0, sizeof smc_host_q);
    proj.set.bt = enabled ? PROJ_BT_SMC_USER : 0;
    int admitted = smc_midi_host(packet(st, a, b));
    int queued = smc_host_w == 1u;
    ++tested;
    if (admitted != expected || queued != expected ||
        (queued && (smc_host_q[0][0] != st || smc_host_q[0][1] != a || smc_host_q[0][2] != b))) {
        ++failed;
        fprintf(stderr, "FAIL %s: admitted=%d queued=%d expected=%d\n", name, admitted, queued, expected);
    }
}
int main(void)
{
    check("valid-solo-on", 0x9F, 8, 127, 1, 1);
    check("valid-solo-off", 0x8F, 8, 0, 1, 1);
    check("valid-user-fader", 0xBF, 40, 127, 1, 1);
    check("valid-user-knob", 0xBF, 16, 1, 1, 1);
    check("user-disabled", 0x9F, 8, 127, 0, 0);
    check("wrong-channel", 0x90, 8, 127, 1, 0);
    check("instrument-note", 0x9F, 36, 127, 1, 0);
    check("invalid-solo-data1", 0x9F, 0x88, 127, 1, 0);
    check("invalid-solo-off-data1", 0x8F, 0x88, 0, 1, 0);
    check("invalid-solo-velocity", 0x9F, 8, 0x80, 1, 0);
    check("invalid-fader-data1", 0xBF, 0xA8, 127, 1, 0);
    check("invalid-fader-data2", 0xBF, 40, 0x80, 1, 0);
    check("unsupported-status", 0xAF, 8, 127, 1, 0);
    printf("SMC source-selected host data7: %u tested, %u failed\n", tested, failed);
    return failed ? 1 : 0;
}
'''


def spill_block(text):
    """Production overflow mailbox for continuous controls."""
    return text[text.index('/* SMC spill begin'):text.index('/* SMC spill end */')]


def main():
    production = SOURCE.read_text()
    src = (HOST + function(PROTOCOL.read_text(), 'smc_host_accept') + '\n'
           + function(PROTOCOL.read_text(), 'smc_relative') + '\n'
           + spill_block(SOURCE.read_text()) + '\n'
           + function(production, 'smc_is_solo_release') + '\n'
           + function(production, 'smc_midi_host') + '\n' + TEST)
    with tempfile.TemporaryDirectory(prefix='smc-host-data7-') as td:
        srcfile, exe = Path(td) / 'test.c', Path(td) / 'test'
        srcfile.write_text(src)
        cc = os.environ.get('CC', 'cc')
        flags = os.environ.get('SAN_FLAGS', '').split()
        cmd = [cc, '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', *flags, str(srcfile), '-o', str(exe)]
        subprocess.run(cmd, check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    main()
