#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""BF-029: exact engine-render callsite, linked production sequencer and motion.

The engine callsite is compiled verbatim, but other engine audio/DSP code is not linked.
The sink simulates the engine s_step call with production motion_step and seq helpers.
"""
import os, pathlib, re, subprocess, sys, tempfile
root=(pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else pathlib.Path(__file__).resolve().parents[2]).resolve()
engine=(root/'firmware/src/app/engine.c').read_text()
render=engine.partition('void engine_render(')[2]
m=re.search(r'\bdrain\(\);(?:(?!PROF_ADD\(7, pt\);).)*PROF_ADD\(7, pt\);',render,re.S)
assert m,'cannot extract engine_render scheduler path'
callsite=m.group().replace('PROF_ADD(7, pt);','')
assert callsite.count('motion_tick(&mot, n);')==1
c=r'''
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "sequencer.h"
#include "motion.h"
static seq_t seq;
static pattern_t pats[NPAT];
static motion_t mot;
static lane_t lanes[NLANE];
static uint8_t mot_on=1, mot_playing=0;
static int fx, trace[128], nt, value, checks, errors, steps;
#define CHECK(C,NAME) do {checks++;if(!(C)){errors++;fprintf(stderr,"FAIL %s: playing=%d age=%u value=%d steps=%d nt=%d\n", NAME,seq.playing,mot.age[0],value,steps,nt);}}while(0)
static int base(void *x,int t,int v,int i) { (void)x;(void)t;(void)v;(void)i;return 0; }
static void apply(void *x,int t,int v,int i,int p) { (void)x;(void)t;(void)v;(void)i;value=p;if(nt<128)trace[nt++]=p; }
static void on_step(void *x,int t,int p) { (void)x;steps++;if(mot_on) motion_step(&mot,t,seq.ppat[t],p,seq_next_step(&seq,t,p),seq_part_len(&seq,t)); }
static void brkstop(void *x) {(void)x;}
static seq_sink_t SINK;
static void drain(void) {}
static void fxbus_set_bpm(int *a,float bpm) {(void)a;(void)bpm;}
static void block(uint32_t n) {
    CALLSITE
    uint32_t left=n;
    while(left) {
        uint32_t delta=seq_until_event(&seq,left);
        if(delta==0 || delta>left) { errors++;fprintf(stderr,"invalid until_event %u of %u\n",delta,left);break; }
        seq_advance(&seq,delta,&SINK);
        left-=delta;
    }
    if(mot_playing && !seq.playing) motion_release(&mot);
    mot_playing=seq.playing;
}
static void reset(void) {
    memset(pats,0,sizeof(pats));
    for(unsigned i=0;i<NPAT;i++)pattern_init(&pats[i],i+17u,i+39u);
    memset(&seq,0,sizeof(seq));seq_init(&seq,pats);
    seq.send_clock=0;seq.send_notes=0;
    memset(&mot,0,sizeof(mot));memset(lanes,0,sizeof(lanes));
    motion_init(&mot,lanes);mot.base=base;mot.apply=apply;
    memset(&SINK,0,sizeof(SINK));SINK.step=on_step;SINK.brk_stop=brkstop;
    int slot=motion_alloc(lanes,0,TRK_DRUM,1,0,0);
    if(slot!=0){errors++;fprintf(stderr,"failed lane alloc %d\n",slot);}
    lanes[slot].val[0]=30;lanes[slot].val[1]=100;
    value=0;nt=0;steps=0;mot_on=1;mot_playing=0;
}
static void initial_play(void) {
    seq_start(&seq);seq_advance(&seq,0,&SINK);
    if(!seq.playing) {errors++;fprintf(stderr,"initial start failed\n");}
    // Trigger real initial step through sequencer without advancing real audio block.
    seq_advance(&seq,0,&SINK);
    // Active interval prepared after actual events for deterministic interpolation stress.
    motion_step(&mot,TRK_DRUM,0,0,1,16);
    mot.len[TRK_DRUM][0]=100;
    mot.age[TRK_DRUM]=0;
    mot_playing=1;
}
static int has(int n) {for(int i=0;i<nt;i++)if(trace[i]==n)return 1;return 0;}
int main(void){
    reset();initial_play();seq_stop(&seq);block(50);
    CHECK(!has(65),"queued STOP must not emit transient 65 ramp");
    CHECK(!seq.playing,"queued STOP must stop");
    CHECK(mot.age[0]==0 && value==0,"queued STOP must release base");
    reset();seq_start(&seq);block(50);
    CHECK(seq.playing,"queued START must start");
    CHECK(mot.age[0]==50,"queued START counts first running block");
    CHECK(mot.len[0][0]==0,"queued START has no previous step length");
    reset();initial_play();block(50);
    CHECK(mot.age[0]==50 && value==65,"regular active motion ramps");
    reset();block(1000);
    CHECK(!seq.playing && mot.age[0]==0,"idle transport freezes age");
    reset();initial_play();seq_stop(&seq);seq_start(&seq);block(50);
    CHECK(seq.playing && mot.age[0]==50,"queued STOP then START counts block");
    reset();seq_continue(&seq);block(50);
    CHECK(seq.playing && mot.age[0]==50,"queued CONTINUE counts block");
    reset();initial_play();mot_on=0;block(50);
    CHECK(mot.age[0]==0,"disabled motion freezes age");
    fprintf(stderr,"BF029 REAL SEQUENCER + MOTION: %d/%d checks, %d failures\n",checks-errors,checks,errors);
    return errors?1:0;
}
'''.replace('CALLSITE',callsite)
cc=os.environ.get('CC','gcc');flags=os.environ.get('CFLAGS','-O2 -Wall -Wextra -Werror').split()
with tempfile.TemporaryDirectory(prefix='bf029-real-') as d:
    src=pathlib.Path(d)/'case.c';out=pathlib.Path(d)/'case';src.write_text(c)
    args=[cc,'-std=c11',*flags,'-I'+str(root/'firmware/src/seq'),str(src)] + [str(root/'firmware/src/seq'/s) for s in ['sequencer.c','pattern.c','tb3po.c','motion.c']] + ['-lm','-o',str(out)]
    p=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    if p.returncode:print('COMPILE exit',p.returncode,p.stdout);sys.exit(p.returncode)
    p=subprocess.run([str(out)],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1'})
    print(p.stdout,end='')
    sys.exit(p.returncode)
