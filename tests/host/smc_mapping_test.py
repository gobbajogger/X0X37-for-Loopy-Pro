#!/usr/bin/env python3
"""Compile production routing and bank logic; cover all eleven drum destinations."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
source = (Path(os.environ.get('SMC_SOURCE_ROOT', root)) / 'firmware/src/app/smc_mixer.c').read_text()
names = ('smc_same', 'smc_home_target', 'smc_page_target', 'smc_mode_target', 'smc_target', 'smc_knob_target', 'smc_bank', 'smc_notice')
functions = []
for helper in ('smc_drum_mix', 'smc_drum_track'):
    match = re.search(r'^static [^\n]+ '+helper+r'\([^\n]*\)\n\{.*?^\}', source, re.M | re.S)
    if match:
        functions.append(match[0])
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
       O_NONE, O_LIST, O_ASK, NPARTS=5, SMC_MAIN=0, SMC_909=1, SMC_808=2 };
typedef struct { uint8_t kind,a,b,c; } pref_t;
#define PR(k,a,b,c) ((pref_t){k,a,b,c})
#define NONE PR(R_NONE,0,0,0)
static struct { int help,overlay,list_top,list_n,view,sel[2]; } ui;
static struct { pref_t r[1][4]; } pg;
static pref_t list_rows[12];
static const char *const PART_N[] = {"909","808"};
static int kit=-1, clip, stage;
static struct { pref_t target[8]; uint8_t value[8],picked,pending,seen,lock,mode; } smc;
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
                if(i!=j) assert(!smc_same(f,smc_target(j)));
            }
            if(k.kind!=R_NONE && i!=j) assert(!smc_same(k,smc_knob_target(j)));
        }
    }
}
int main(void) {
    /* Owner: include knobs in all three mixer modes. */
    ui.view=V_HOME; ui.overlay=O_NONE;
    for(int n=0;n<8;n++) assert(smc_same(smc_knob_target(n),smc_home_target(n)));
    unique();
    ui.view=V_MIX; ui.overlay=O_NONE;
    for(int n=0;n<8;n++) assert(smc_same(smc_knob_target(n),smc_home_target(n)));
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
            for(int n=0;n<8;n++) assert(smc_same(smc_knob_target(n),smc_target(n)));
            unique();
        }
        assert(seen==0x7FF);
        ui.sel[kit]=10; build_pages(); assert(smc_target(2).b==10); unique();
        ui.sel[kit]=7; build_pages(); assert(smc_target(7).b==7); unique();
    }
    ui.view=V_PART; kit=-1;
    pg.r[0][0]=PR(R_ENG,T_MIX,0,MX_LEVEL);
    assert(smc_target(0).kind==R_ENG && smc_same(smc_knob_target(0),pg.r[0][0]));
    pg.r[0][1]=pg.r[0][0]; assert(smc_knob_target(1).kind==R_NONE);
    pg.r[0][1]=PR(R_ENG,T_MIX,0,1);
    unique(); assert(!smc_bank(0x2F));
    ui.overlay=O_LIST; ui.list_n=8; ui.list_top=2;
    for(int n=0;n<12;n++) list_rows[n]=PR(R_ENG,T_MIX,n,1);
    for(int n=0;n<8;n++) {
        assert(smc_same(smc_target(n),smc_home_target(n)));
        assert(n<6 ? smc_knob_target(n).b==n+2 : smc_knob_target(n).kind==R_NONE);
    }
    unique();
    ui.help=1;
    for(int n=0;n<8;n++) assert(smc_target(n).kind==R_NONE && smc_knob_target(n).kind==R_NONE);
    ui.help=0; ui.overlay=O_NONE; clip=1; stage=1;
    for(int n=0;n<8;n++) assert(smc_same(smc_target(n),smc_home_target(n)));
    /* Lock off still follows HOME, main MIX, the 11-track bank, a menu and a sound page. */
    smc.lock=0; smc.mode=SMC_MAIN; clip=0; kit=-1; ui.view=V_HOME; ui.overlay=O_NONE;
    for(int n=0;n<8;n++) {
        assert(smc_same(smc_target(n),smc_home_target(n)));
        assert(smc_same(smc_knob_target(n),smc_home_target(n)));
    }
    ui.view=V_MIX;
    for(int n=0;n<8;n++) assert(smc_same(smc_knob_target(n),smc_home_target(n)));
    ui.view=V_MIX; kit=0; ui.sel[0]=0;
    assert(smc_bank(0x2F) && ui.sel[0]==8);
    assert(smc_target(0).a==T_909 && smc_target(0).b==8 && smc_target(3).kind==R_NONE);
    ui.view=V_PART; kit=-1; ui.overlay=O_NONE;
    pg.r[0][0]=PR(R_ENG,T_MIX,1,2);
    assert(smc_same(smc_target(0),smc_home_target(0)));
    assert(smc_same(smc_knob_target(0),pg.r[0][0]));
    ui.overlay=O_LIST; ui.list_n=1; ui.list_top=0; list_rows[0]=PR(R_ENG,T_MIX,3,1);
    assert(smc_same(smc_target(0),smc_home_target(0)) && smc_knob_target(0).b==3);
    /* A 909 lock keeps that kit after the view leaves the MIX page. */
    smc.lock=1; smc.mode=SMC_909; ui.overlay=O_NONE; ui.help=0; ui.view=V_PART; ui.sel[0]=0;
    for(int n=0;n<8;n++) {
        pref_t r=smc_target(n);
        assert(r.a==T_909 && r.b==n && smc_same(smc_knob_target(n),r));
    }
    ui.view=V_HOME;
    assert(smc_target(3).a==T_909 && smc_target(3).b==3);
    assert(smc_bank(0x2F) && ui.sel[0]==8);
    assert(smc_target(0).a==T_909 && smc_target(0).b==8 && smc_target(2).b==10);
    assert(smc_target(3).kind==R_NONE);
    smc.mode=SMC_808; ui.sel[1]=0; ui.view=V_HOME;
    assert(smc_bank(0x2E) && ui.sel[1]==0);
    assert(smc_target(1).a==T_808 && smc_target(1).b==1);
    assert(smc_knob_target(1).a==T_808);
    /* A lock taken on HOME keeps the eight main-mix strips after a sound page opens. */
    smc.lock=1; smc.mode=SMC_MAIN; ui.view=V_PART; ui.overlay=O_NONE;
    pg.r[0][0]=PR(R_ENG,T_909,4,1);
    for(int n=0;n<8;n++) {
        assert(smc_same(smc_target(n),smc_home_target(n)));
        assert(smc_same(smc_knob_target(n),smc_home_target(n)));
    }
    smc.lock=0;
    assert(smc_same(smc_knob_target(0),pg.r[0][0]));
    assert(smc_same(smc_target(0),smc_home_target(0)));
    smc.lock=1; smc.mode=SMC_909; ui.help=1;
    for(int n=0;n<8;n++) assert(smc_target(n).kind==R_NONE && smc_knob_target(n).kind==R_NONE);
    assert(!smc_bank(0x2F));
    ui.help=0; ui.overlay=O_ASK;
    assert(smc_target(0).kind==R_NONE && smc_knob_target(0).kind==R_NONE);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c=Path(tmp)/'test.c'; exe=Path(tmp)/'test'; c.write_text(harness)
    subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror','-I'+str(root/'firmware/src'),str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: both drum kits, 11 tracks, bank bounds, three knob modes, unique targets within each type, menus and stage isolation')

button = re.search(r'^static void smc_button\([^\n]*\)\n\{.*?^\}', source, re.M | re.S)
assert button, 'smc_button'
pause_helpers = []
for helper in ('smc_drum_mix', 'smc_drum_track', 'smc_bank'):
    match = re.search(r'^static [^\n]+ '+helper+r'\([^\n]*\)\n\{.*?^\}', source, re.M | re.S)
    assert match, helper
    pause_helpers.append(match[0])
pause = r'''
#include <assert.h>
#include <stdint.h>
enum { NPARTS=5, NPAT=16, NDRUM=11, V_FX=20, V_HOME, V_MIX, V_PART, V_SEQ, V_GEN, V_SONG, V_GLO,
       O_NONE, O_LIST, O_ASK, B_SEL, B_HOME, B_GLO, SMC_MAIN=0, SMC_909=1, SMC_808=2 };
static struct { int part, overlay, view, song_sel, help, sel[2]; } ui;
static struct { int solo; uint8_t lock, mode; } smc;
static struct { int ppat[NPARTS], playing, song_start; } seq;
static struct { int len; } song_data;
static struct { int n; } pg;
static int kit, drum_part, stops, plays;
static const char *const PART_N[] = {"909","808","A","B","BREAK"};
#define song() (&song_data)
static int smk_ck_smc_nav(uint8_t note) { (void)note; return 0; }
static int smk_clip_mode_on(void) { return 0; }
static int drum_mix_page(void) { return ui.view==V_MIX ? kit : -1; }
static int is_drum(void) { return drum_part; }
static int is_303(void) { return 0; }
static void build_pages(void) {}
static void button_tap(int b) { (void)b; }
static void smc_select_part(int p) { ui.part=p; }
static void smc_record_toggle(void) {}
static uint8_t smc_solo_mask(void) { return (uint8_t)smc.solo; }
static void engine_smc_solo(int s) { (void)s; }
static void say(const char *a, const char *b) { (void)a; (void)b; }
static void mute_part(int p) { (void)p; }
#define mute_track(k,t) ((void)(k),(void)(t))
static void smc_toggle_fx(int n) { (void)n; }
static void smc_view(int v, int p) { ui.view=v; (void)p; }
#define seq_cue_part(s,p,n) ((s)->ppat[p]=(n))
#define seq_stop(s) do { (void)(s); stops++; } while (0)
#define seq_start(s) do { (s)->playing=1; plays++; } while (0)
static void list_move(int d) { (void)d; }
static void ui_menu_arrow(int d) { (void)d; }
static int cur_page(void) { return 0; }
static uint8_t page_store;
static uint8_t *page_of(int v) { (void)v; return &page_store; }
''' + '\n'.join(pause_helpers) + '\n' + button[0] + r'''
int main(void) {
    ui.view=V_HOME; ui.overlay=O_NONE; seq.playing=1; kit=-1;
    smc_button(0x5D, 0);
    assert(smc.lock==1 && smc.mode==SMC_MAIN && stops==0 && seq.playing==1);
    smc_button(0x5D, 0);
    assert(smc.lock==0 && stops==0 && seq.playing==1);
    ui.view=V_MIX; kit=0;
    smc_button(0x5D, 0);
    assert(smc.lock==1 && smc.mode==SMC_909 && stops==0);
    smc.lock=0; kit=1;
    smc_button(0x5D, 0);
    assert(smc.lock==1 && smc.mode==SMC_808);
    ui.view=V_PART; drum_part=1; ui.sel[1]=3;
    smc_button(0x2F, 0);
    assert(ui.sel[1]==8);
    seq.playing=0;
    smc_button(0x5E, 0);
    assert(seq.playing==1 && plays==1 && stops==0);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c=Path(tmp)/'pause.c'; exe=Path(tmp)/'pause'; c.write_text(pause)
    subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror','-I'+str(root/'firmware/src'),str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: Pause 0x5D toggles the lock, stores the visible mix, keeps a locked bank, and does not stop playback')
