#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Independent read-only SMC User MIDI overflow/admission regression.

Extract actual production SMC functions, compile against an isolated synthetic
queue. No hardware, firmware compilation, MIDI writes or private inputs.
"""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(os.environ.get('SMC_SOURCE', ROOT / 'firmware/src/app/smc_mixer.c'))
PROTOCOL = Path(os.environ.get('SMC_PROTOCOL', ROOT / 'firmware/src/app/smc_protocol.h'))


def function(source, name):
    m = re.search(r'^[^\n]*\b' + re.escape(name) + r'\([^\n]*\)\s*\{', source, re.M)
    if not m:
        raise ValueError('Missing actual production function: ' + name)
    depth, end = 1, m.end()
    while depth:
        if end >= len(source):
            raise ValueError('Unterminated function: ' + name)
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[m.start():end]


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

CASES = r'''
static unsigned cases, failed;
static uint32_t packet(uint8_t st, uint8_t a, uint8_t b)
{
    return 9u | ((uint32_t)st << 8) | ((uint32_t)a << 16) | ((uint32_t)b << 24);
}
static void scenario(const char *name, uint8_t st, uint8_t a, uint8_t b,
                     unsigned user, uint8_t w, uint8_t r,
                     unsigned accepted, uint8_t w_after, uint32_t release_lost)
{
    ++cases;
    memset(smc_host_q, 0xFE, sizeof smc_host_q);
    proj.set.bt = user ? PROJ_BT_SMC_USER : 0;
    smc_host_w = w;
    smc_host_r = r;
    smc_host_solo_release_lost = 0;
    unsigned ret = (unsigned)smc_midi_host(packet(st, a, b));
    unsigned wrote = w_after != w;
    int bad = ret != accepted || smc_host_w != w_after ||
              smc_host_solo_release_lost != release_lost;
    uint8_t i = w % SMC_Q;
    if (wrote) {
        bad |= smc_host_q[i][0] != st || smc_host_q[i][1] != a || smc_host_q[i][2] != b;
    } else {
        bad |= smc_host_q[i][0] != 0xFE || smc_host_q[i][1] != 0xFE || smc_host_q[i][2] != 0xFE;
    }
    if (bad) {
        ++failed;
        fprintf(stderr, "FAIL %s ret=%u/%u w=%u/%u lost=%u/%u queue=%02X-%02X-%02X\n",
                name, ret, accepted, smc_host_w, w_after,
                (unsigned)smc_host_solo_release_lost, (unsigned)release_lost,
                smc_host_q[i][0], smc_host_q[i][1], smc_host_q[i][2]);
    }
}
int main(void)
{
    /* Admission and ring fullness are distinct: return 1 can mean dropped. */
    scenario("solo-off-room", 0x8F, 8, 0, 1, 0, 0, 1, 1, 0);
    scenario("solo-off-last-free", 0x8F, 8, 0, 1, 31, 0, 1, 32, 0);
    scenario("solo-off-full", 0x8F, 8, 0, 1, 32, 0, 1, 32, 1);
    scenario("solo-vel0-full", 0x9F, 8, 0, 1, 32, 0, 1, 32, 1);
    scenario("solo-on-full", 0x9F, 8, 127, 1, 32, 0, 1, 32, 0);
    scenario("home-off-full", 0x8F, 6, 0, 1, 32, 0, 1, 32, 1);
    scenario("home-vel0-full", 0x9F, 6, 0, 1, 32, 0, 1, 32, 1);
    scenario("home-on-full", 0x9F, 6, 127, 1, 32, 0, 1, 32, 0);
    /* Every held SMC button needs a key-up, not just Solo. A dropped key-up
     * must advance the shared generation so poll clears its retained press. */
    scenario("non-solo-off-full", 0x8F, 7, 0, 1, 32, 0, 1, 32, 1);
    scenario("non-solo-boundary-full", 0x8F, 16, 0, 1, 32, 0, 1, 32, 1);
    scenario("select-off-full", 0x8F, 24, 0, 1, 32, 0, 1, 32, 1);
    scenario("solo-end-full", 0x8F, 15, 0, 1, 32, 0, 1, 32, 1);
    scenario("user-cc-full", 0xBF, 40, 127, 1, 32, 0, 1, 32, 0);
    scenario("malformed-solo-note-full", 0x8F, 0x88, 0, 1, 32, 0, 0, 32, 0);
    scenario("malformed-solo-velocity-full", 0x8F, 8, 0x80, 1, 32, 0, 0, 32, 0);
    scenario("non-user-channel-full", 0x80, 8, 0, 1, 32, 0, 0, 32, 0);
    scenario("disabled-user-full", 0x8F, 8, 0, 0, 32, 0, 0, 32, 0);
    scenario("invalid-status-full", 0xAF, 8, 0, 1, 32, 0, 0, 32, 0);
    scenario("full-after-wrap", 0x8F, 8, 0, 1, 0, 224, 1, 0, 1);
    scenario("last-free-wrap", 0x8F, 8, 0, 1, 255, 224, 1, 0, 0);
    printf("SMC overflow/user-data7 source-exact: %u cases, %u failures\n", cases, failed);
    return failed ? 1 : 0;
}
'''


def spill_block(text):
    """Production overflow mailbox for continuous controls."""
    return text[text.index('/* SMC spill begin'):text.index('/* SMC spill end */')]


def main():
    src = (HOST + function(PROTOCOL.read_text(), 'smc_host_accept') + '\n'
           + function(PROTOCOL.read_text(), 'smc_relative') + '\n'
           + spill_block(SOURCE.read_text()) + '\n'
           + function(SOURCE.read_text(), 'smc_is_solo_release') + '\n'
           + function(SOURCE.read_text(), 'smc_midi_host') + '\n' + CASES)
    with tempfile.TemporaryDirectory(prefix='smc-host-overflow-') as tmp:
        cfile = Path(tmp, 'regression.c')
        executable = Path(tmp, 'regression')
        cfile.write_text(src)
        flags = os.environ.get('SAN_FLAGS', '').split()
        command = [os.environ.get('CC', 'cc'), '-std=c99', '-O1', '-g',
                   '-Wall', '-Wextra', '-Werror', *flags,
                   str(cfile), '-o', str(executable)]
        subprocess.run(command, check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == '__main__':
    main()
