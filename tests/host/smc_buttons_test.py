#!/usr/bin/env python3
"""Exercise production SMC strip shortcuts and the four navigation arrows."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
s = (root / 'firmware/src/app/smc_mixer.c').read_text()
f = re.search(r'^static void smc_button\([^\n]*\)\n\{.*?^\}', s, re.M | re.S)[0]
harness = r'''
#include <assert.h>
#include <stdint.h>
enum { NPARTS=5, NPAT=16, V_FX=20, V_HOME, V_MIX, V_PART, O_NONE, O_LIST };
static struct { int part, overlay, view, song_sel; } ui;
static struct { int solo; } smc;
static struct { int ppat[NPARTS], playing, song_start; } seq;
static struct { int len; } song_data;
static struct { int n; } pg;
static uint8_t page;
static const char *const PART_N[] = {"909","808","A","B","BREAK"};
static int last_view,last_page,fx,moved,edited;
#define song() (&song_data)
static void smc_select_part(int p) { ui.part=p; }
static void smc_record_toggle(void) {}
static void engine_smc_solo(int s) { (void)s; }
static void say(const char *a,const char *b) { (void)a;(void)b; }
static void mute_part(int p) { (void)p; }
static void smc_toggle_fx(int n) { fx=n; }
static void smc_view(int v,int p) { last_view=v; last_page=p; }
static int smc_bank(uint8_t n) { (void)n; return 0; } /* bank logic has its own routing test */
#define seq_cue_part(s,p,n) ((s)->ppat[p]=(n))
#define seq_stop(s) ((s)->playing=0)
#define seq_start(s) ((s)->playing=1)
static void list_move(int d) { moved+=d; }
static void ui_menu_arrow(int d) { edited+=d; }
static int cur_page(void) { return page; }
static uint8_t *page_of(int v) { (void)v; return &page; }
'''+f+r'''
int main(void) {
    const int views[4][3]={{V_FX,V_FX,V_HOME},{V_MIX,V_MIX,V_PART},{-1,-1,-1},{V_FX,V_FX,V_MIX}};
    const int pages[4][3]={{0,1,0},{-1,-1,0},{-1,-1,-1},{0,1,3}};
    for(int row=0;row<4;row++) for(int strip=5;strip<8;strip++) {
        last_view=last_page=fx=-1;
        smc_button(row*8+strip,0);
        if(row==2) assert(fx==strip-5 && last_view==-1);
        else assert(last_view==views[row][strip-5] && last_page==pages[row][strip-5] && fx==-1);
    }
    pg.n=4;
    for(int note=0x60;note<=0x63;note++) {
        int d=(note&1)?1:-1;
        ui.overlay=O_NONE; page=1; smc_button(note,0); assert(page==1+d);
        page=d<0?0:3; smc_button(note,0); assert(page==(d<0?0:3));
        ui.overlay=O_LIST; moved=edited=0; smc_button(note,0);
        assert(note<0x62 ? moved==d && edited==0 : moved==0 && edited==d);
    }
    return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c=Path(tmp)/'test.c'; exe=Path(tmp)/'test'; c.write_text(harness)
    subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: strip shortcuts and page/menu arrows retain their mappings')
