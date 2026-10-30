#!/usr/bin/env python3
"""Exercise production SMC strip shortcuts and the four navigation arrows."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
s = Path(os.environ.get('SMC_SOURCE', Path(os.environ.get('SMC_SOURCE_ROOT', root)) / 'firmware/src/app/smc_mixer.c')).read_text()
# Mapping remains in apply(); the full UI regression exercises its presentation wrapper.
f = re.search(r'^static void smc_button(?:_apply)?\([^\n]*\)\n\{.*?^\}', s, re.M | re.S)[0]
f = f.replace('smc_button_apply(', 'smc_button(', 1)
helpers = [re.search(r'^static const (?:uint8_t|int8_t) '+name+r'\[4\]\[3\] = \{.*?^\};', s, re.M | re.S)[0]
           for name in ('SMC_VIEW', 'SMC_PAGE')]
for name in ('smc_drum_mix', 'smc_drum_track', 'smc_lock_blocks', 'smc_nav_key'):
    match = re.search(r'^static [^\n]+ '+name+r'\([^\n]*\)\n\{.*?^\}', s, re.M | re.S)
    if match:
        helpers.append(match[0])
ui_source = (root / 'firmware/src/app/ui.c').read_text()
order_helpers = 'enum { PART_909=0, PART_808=1, PART_303A=2, PART_303B=3, PART_BRK=4 };\n'
order_helpers += re.search(r'^static const uint8_t PART_ORDER.*?;', ui_source, re.M)[0] + '\n'
order_helpers += re.search(r'^static int part_at\(.*', ui_source, re.M)[0] + '\n'
for helper in ('part_position', 'part_neighbor'):
    order_helpers += re.search(r'^static int '+helper+r'\([^\n]*\)\n\{.*?^\}', ui_source, re.M|re.S)[0] + '\n'
harness = r'''
#include <assert.h>
#include <stdint.h>
#include "app/smc_mixer_edges.h"
#include "smc_fx_harness_stubs.h"
#define NKIT 2
enum { NPAT=16, V_FX=20, V_HOME, V_MIX, V_PART, V_SEQ, V_GEN, V_SONG, V_GLO, O_NONE, O_LIST, O_ASK, B_SEL, B_HOME, B_GLO, NDRUM=11, T_909, T_808 };
static struct { int part, overlay, view, song_sel, help, sel[2]; } ui;
static struct { int solo,locked,lock_kit,lock_bank,lock_part,lock_view,lock_page,lock_pages; uint8_t lock_page_kit[MAXPAGES]; uint16_t drum_solo[2]; uint8_t solo_target[8]; } smc;
static struct { int ppat[NPARTS], pcue[NPARTS], playing, song_start, req_start, req_stop, req_cont; } seq;
static struct { int len; } song_data;
static struct { int n; const char *title[MAXPAGES]; } pg;
static uint8_t page;
static const char *const PART_N[] = {"909","808","A","B","BREAK"};
static int last_view,last_page,fx,moved,edited,tap,knob_bank,home_held;
static int smc_home_held(void) { return home_held; }
static void smc_knob_bank(int d) { knob_bank+=d; }
static int smc_page_now(void) { return page; }
static void smc_page_set(int p) { page=(uint8_t)p; }
static void button_tap(int b) { tap=b; }
static int smk_ck_smc_nav(uint8_t note) { (void)note; return 0; }
static int smk_clip_mode_on(void) { return 0; }
static int is_drum(void) { return 0; }
static int is_303(void) { return 1; }
static void build_pages(void) {}
#define song() (&song_data)
static void smc_select_part(int p) { ui.part=p; }
static void smc_record_toggle(void) {}
#define engine_smc_solo(s) ((void)(s))
#define smc_solo_press(s,t,m) ((void)(s),(void)(t),(void)(m))
#define smc_select_drum(k,t) ((void)(k),(void)(t),build_pages())
#define engine_voice_name(k,t) ((void)(k),(void)(t),"drum")
static void say(const char *a,const char *b) { (void)a;(void)b; }
static void ui_say(const char *a,const char *b) { (void)a;(void)b; }
static void mute_part(int p) { (void)p; }
#define mute_track(k,t) ((void)(k),(void)(t))
static void smc_toggle_fx(int n) { fx=n; }
static int smc_fx_try(uint8_t note, int mmc) { (void)note; (void)mmc; return 0; }
static void smc_view(int v,int p) { last_view=v; last_page=p; }
static int smc_bank(uint8_t n) { (void)n; return 0; } /* bank logic has its own routing test */
#define seq_cue_part(s,p,n) ((s)->pcue[p]=(n))
#define seq_cue_of(s,p) ((s)->pcue[p]>=0 && (s)->pcue[p]<NPAT ? (s)->pcue[p] : -1)
#define seq_stop(s) ((s)->playing=0)
#define seq_start(s) ((s)->playing=1)
static void list_move(int d) { moved+=d; }
static void ui_menu_arrow(int d) { edited+=d; }
'''+ order_helpers + '\n'.join(helpers) + f + r'''
int main(void) {
    /* Current production map: R5/R6 tap SELECT/HOME, R14 taps GLOBAL; strip 7
     * toggles HOME/SONG (row 0), PART/SEQ (row 1) and opens the mixer (row 3). */
    const int views[4][3]={{-1,-1,V_HOME},{V_MIX,-1,V_PART},{-1,-1,-1},{V_FX,V_FX,V_MIX}};
    const int pages[4][3]={{-1,-1,-1},{-1,-1,-1},{-1,-1,-1},{0,1,0}};
    pg.n=4;
    for(int row=0;row<4;row++) for(int strip=5;strip<8;strip++) {
        last_view=last_page=fx=-1; ui.view=0;
        int note=row*8+strip; tap=-1; smc_button(note,0);
        if(note==5 || note==6 || note==14) {
            assert(tap==(note==5?B_SEL:note==6?B_HOME:B_GLO)); continue;
        }
        if(row==2) assert(fx==strip-5 && last_view==-1);
        else assert(last_view==views[row][strip-5] && last_page==pages[row][strip-5] && fx==-1);
    }
    for(int note=0x60;note<=0x63;note++) {
        int d=(note&1)?1:-1;
        ui.overlay=O_NONE; ui.view=V_PART; page=1; knob_bank=0; smc_button(note,0);
        if(note<0x62) assert(knob_bank==d && page==1);
        else assert(knob_bank==0 && page==1+d);
        ui.overlay=O_LIST; moved=edited=0; knob_bank=0; smc_button(note,0);
        assert(note<0x62 ? moved==d && edited==0 : moved==0 && edited==d);
        assert(knob_bank==0);
    }
    return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c=Path(tmp)/'test.c'; exe=Path(tmp)/'test'; c.write_text(harness)
    subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror',
                    '-I'+str(root/'firmware'/'src'), '-I'+str(root/'tests'/'host'),
                    str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: strip shortcuts, knob-bank arrows and page/menu arrows use explicit view/page/menu mappings')
