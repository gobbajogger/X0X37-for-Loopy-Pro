#!/usr/bin/env python3
"""Compile production routing and bank logic; cover all eleven drum destinations."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
source = (root / 'firmware/src/app/smc_mixer.c').read_text()
names = ('smc_same', 'smc_home_target', 'smc_page_target', 'smc_target', 'smc_knob_target', 'smc_bank', 'smc_notice')
functions = []
for name in names:
    match = re.search(r'^static [^\n]+ '+name+r'\([^\n]*\)\n\{.*?^\}', source, re.M | re.S)
    assert match, name
    functions.append(match[0])
harness = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "seq/pattern.h"
enum { R_NONE, R_ENG, R_HDR, R_ACT, T_909, T_808, T_MIX, T_FX, T_MST,
       MX_LEVEL, FX_RV_LEVEL, FX_DL_LEVEL, MST_DJF, V_HOME, V_MIX, V_PART,
       O_NONE, O_LIST, O_ASK, NPARTS=5 };
typedef struct { uint8_t kind,a,b,c; } pref_t;
#define PR(k,a,b,c) ((pref_t){k,a,b,c})
#define NONE PR(R_NONE,0,0,0)
static struct { int help,overlay,list_top,list_n,view,sel[2]; } ui;
static struct { pref_t r[1][4]; } pg;
static pref_t list_rows[12];
static const char *const PART_N[] = {"909","808"};
static int kit=-1, clip, stage;
static struct { pref_t target[8]; uint8_t value[8],picked,pending,seen; } smc;
static uint8_t smc_scaled(pref_t r) { return r.kind==R_NONE ? 0 : 64; }
static int cur_page(void) { return 0; }
static int drum_mix_page(void) { return ui.view==V_MIX ? kit : -1; }
static int smk_clip_mode_on(void) { return clip; }
static pref_t find_ref(int t,int track,const char *name) {
    assert(track>=0 && track<NDRUM && !strcmp(name,"Level"));
    return PR(R_ENG,t,track,0);
}
static void build_pages(void) {
    for(int n=0;n<4;n++) pg.r[0][n]=PR(R_ENG,kit ? T_808 : T_909,ui.sel[kit],n);
}
static void say(const char *a,const char *b) { (void)a;(void)b; }
'''
harness += '\n'.join(functions)
harness += r'''
static void unique(void) {
    for(int i=0;i<8;i++) {
        pref_t f=smc_target(i), k=smc_knob_target(i);
        for(int j=0;j<8;j++) {
            if(f.kind!=R_NONE) {
                assert(!smc_same(f,smc_knob_target(j)));
                if(i!=j) assert(!smc_same(f,smc_target(j)));
            }
            if(k.kind!=R_NONE && i!=j) assert(!smc_same(k,smc_knob_target(j)));
        }
    }
}
int main(void) {
    ui.view=V_MIX; ui.overlay=O_NONE;
    for(kit=0;kit<2;kit++) {
        unsigned seen=0;
        for(int bank=0;bank<2;bank++) {
            assert(smc_bank(bank ? 0x2F : 0x2E));
            smc.picked=255; smc.pending=0;
            for(int n=0;n<8;n++) smc_notice(n);
            assert(smc.picked==0 && smc.pending==255);
            for(int n=0;n<8;n++) {
                pref_t r=smc_target(n);
                if(bank && n>=3) { assert(r.kind==R_NONE); continue; }
                assert(r.kind==R_ENG && r.a==(kit ? T_808:T_909) && r.b==bank*8+n && r.c==0);
                seen |= 1u<<r.b;
            }
            assert(smc_knob_target(0).kind==R_NONE);
            assert(smc_knob_target(1).c==1);
            unique();
        }
        assert(seen==0x7FF);
        ui.sel[kit]=10; build_pages(); assert(smc_target(2).b==10); unique();
        ui.sel[kit]=7; build_pages(); assert(smc_target(7).b==7); unique();
    }
    ui.view=V_PART; kit=-1;
    pg.r[0][0]=PR(R_ENG,T_MIX,0,MX_LEVEL);
    assert(smc_target(0).kind==R_ENG && smc_knob_target(0).kind==R_NONE);
    unique(); assert(!smc_bank(0x2F));
    ui.overlay=O_LIST; ui.list_n=8; ui.list_top=2;
    for(int n=0;n<12;n++) list_rows[n]=PR(R_ENG,T_MIX,n,1);
    for(int n=0;n<8;n++) {
        assert(smc_same(smc_target(n),smc_home_target(n)));
        assert(n<6 ? smc_knob_target(n).b==n+2 : smc_knob_target(n).kind==R_NONE);
    }
    unique();
    ui.help=1;
    for(int n=0;n<8;n++) assert(smc_target(n).kind==R_NONE);
    ui.help=0; ui.overlay=O_NONE; clip=1; stage=1;
    for(int n=0;n<8;n++) assert(smc_same(smc_target(n),smc_home_target(n)));
    return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c=Path(tmp)/'test.c'; exe=Path(tmp)/'test'; c.write_text(harness)
    subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror','-I'+str(root/'firmware/src'),str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: both drum kits, 11 tracks, bank bounds, unique fader/knob targets, menus and stage isolation')
