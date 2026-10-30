#!/usr/bin/env python3
"""Verify production fader pickup, writes and one final-value display per event."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
s = (root / 'firmware/src/app/smc_mixer.c').read_text()
f = re.search(r'^static void smc_fader\([^\n]*\)\n\{.*?^\}', s, re.M | re.S)[0]
harness = r'''
#include <assert.h>
#include <stdint.h>
#include "app/smc_protocol.h"
enum { R_NONE, R_ACT, R_HDR, R_ENG };
typedef struct { uint8_t kind; } pref_t;
typedef struct { int max; } x0x_param_t;
static struct { pref_t target[8]; uint8_t value[8],last[8],seen,picked,pending,input_seen; } smc;
static struct { uint32_t act_t; } ui;
static x0x_param_t desc={127};
static int value=64,writes,shows,shown,motions;
static void smc_notice(int n) { (void)n; }
static const x0x_param_t *pref_desc(pref_t r) { (void)r; return &desc; }
static int pref_get(pref_t r) { (void)r; return value; }
static void pref_set(pref_t r,int v) { (void)r; value=v; writes++; }
static uint8_t smc_scaled(pref_t r) { (void)r; return value; }
static void motion_touch(pref_t r) { (void)r; motions++; }
static int volume_muted(pref_t r) { (void)r; return 0; }
static void smc_show(int n,int knob,pref_t r) { (void)n;(void)knob;(void)r; shows++;shown=value; }
static uint32_t plat_ms(void) { return 1234; }
'''+f+r'''
int main(void) {
    smc.target[0].kind=R_ENG; smc.value[0]=64;
    smc_fader(0,0);
    assert(!writes && !smc.picked && shows==1 && shown==64 && ui.act_t==1234);
    shows=0; smc_fader(0,127);
    assert(writes==1 && motions==1 && smc.picked==1 && shows==1 && shown==127 && smc.pending==1);
    shows=0; smc_fader(0,126);
    assert(writes==2 && motions==2 && shows==1 && shown==126);
    shows=0; smc_fader(0,126);
    assert(writes==2 && shows==0 && shown==126);
    shows=0; smc_fader(8,0); smc_fader(-1,0);
    smc.target[0].kind=R_NONE; smc_fader(0,0);
    assert(shows==0 && writes==2);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c=Path(tmp)/'test.c'; exe=Path(tmp)/'test'; c.write_text(harness)
    subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror','-I'+str(root/'firmware/src'),str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: fader pickup, motion, inactive inputs and one final-value display')
