#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile the actual SELECT handler in a bounded host state scaffold.

Not full UI/DSP/render proof: input sources and side-effect handlers are spies.
UI_SOURCE may select an immutable baseline; CC and SAN_FLAGS configure compiler.
"""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = Path(os.environ.get('UI_SOURCE', root / 'firmware/src/app/ui.c'))
text = source.read_text()
start = text.index('    if ((e = enc(EN_SELECT)) != 0) {')
end = text.index('    if ((e = enc(EN_ALGO)) != 0) {', start)
handler = text[start:end]
# These production ownership guards must still precede the SELECT handler.
prefix = text[text.rfind('    if (ui.overlay == O_ASK)', 0, start):start]
assert 'if (perf_testing())' in prefix and prefix.count('return;') >= 2
assert 'turn_ref(' not in handler and 'ask(' not in handler
scaffold = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
enum { V_PART, V_MIX, V_FX, V_HOME, V_SEQ, V_GEN, V_SONG, V_PERF, V_GLO, V_COUNT };
enum { O_NONE, O_LIST }; enum { EN_SELECT, B_HOME };
struct { int view, part, overlay; unsigned btn, btn_used, touched, touch_until; } ui;
struct { int n; } pg;
struct { float bpm; int playing; } seq;
static uint8_t pages[V_COUNT];
static int input, held = -1, probability, listed, dirty, failures, checks;
static unsigned project_guard = 0x12345678;
static int enc(int n) { (void)n; int e=input; input=0; return e; }
static int held_any_step(void) { return held; }
static void prob_turn(int e) { probability += e; }
static int accel(int n,int e,int limit) { (void)n;(void)limit;return e; }
static unsigned plat_ms(void) { return 100; }
static void mark_dirty(void) { dirty++; }
static void list_move(int d) { listed += d; }
static uint8_t *page_of(int v) { return &pages[v]; }
static int cur_page(void) { return pages[ui.view]; }
static int is_drum(void) { return ui.part < 2; }
static int is_303(void) { return ui.part == 2 || ui.part == 3; }
static void tick(void) { int e;
'''
tests = r'''
}
#define CHECK(x) do { checks++; if(!(x)){ if(failures<12) fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); failures++; }}while(0)
static void reset(void) { memset(&ui,0,sizeof ui); memset(pages,0,sizeof pages); held=-1; probability=listed=dirty=0; seq.bpm=120;seq.playing=1; }
int main(void) {
 for(int view=0;view<V_COUNT;view++) for(int part=0;part<5;part++)
 for(int n=1;n<=14;n++) for(int page=0;page<n;page++) for(int d=-1;d<=1;d+=2) {
  reset();ui.view=view;ui.part=part;pg.n=n;pages[view]=(uint8_t)page;
  int wraps=view==V_MIX||view==V_FX||(view==V_PART&&part<4);
  int want=page+d;if(want<0)want=wraps?n-1:0;if(want>=n)want=wraps?0:n-1;
  input=d*7;tick();CHECK(cur_page()==want);CHECK(ui.part==part);
  CHECK(!dirty&&!listed&&!probability);CHECK(seq.bpm==120&&seq.playing==1);CHECK(project_guard==0x12345678);
  for(int k=0;k<V_COUNT;k++) if(k!=view)CHECK(pages[k]==0);
 }
 reset();ui.view=V_MIX;pg.n=5;for(int k=0;k<25;k++){input=1;tick();CHECK(cur_page()==(k+1)%5);}
 reset();ui.view=V_FX;pg.n=2;held=3;input=-1;tick();CHECK(probability==-1&&cur_page()==0&&!dirty);
 reset();ui.view=V_MIX;pg.n=5;ui.overlay=O_LIST;input=1;tick();CHECK(listed==1&&cur_page()==0&&!dirty);
 reset();ui.view=V_MIX;pg.n=5;held=3;ui.btn=1u<<B_HOME;seq.bpm=275;input=1;tick();CHECK(seq.bpm==275&&dirty==1&&!probability&&cur_page()==0);
 seq.bpm=20;input=-1;tick();CHECK(seq.bpm==20&&cur_page()==0);
 reset();ui.view=V_MIX;pg.n=5;input=0;tick();CHECK(cur_page()==0&&!dirty&&!listed&&!probability);
 printf("SELECT extraction: %d checks, %d failures; routing helpers are spies, full UI NOT_RUN\n",checks,failures);
 return failures?1:0;
}
'''
with tempfile.TemporaryDirectory(prefix='ui-page-wrap-') as tmp:
    c = Path(tmp)/'test.c'; exe = Path(tmp)/'test'
    c.write_text(scaffold + handler + tests)
    cmd = shlex.split(os.environ.get('CC','cc')) + ['-std=c99','-O1','-Wall','-Wextra'] + shlex.split(os.environ.get('SAN_FLAGS','')) + [str(c),'-o',str(exe)]
    subprocess.run(cmd,check=True)
    subprocess.run([str(exe)],check=True)
