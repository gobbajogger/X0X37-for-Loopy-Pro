/* SPDX-License-Identifier: GPL-3.0-only */
/* Source-linked 303 TB-3PO mutation/probability-bar semantics across directions. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "sequencer.h"

static pattern_t pat[NPAT];
static unsigned checks, failed, played;
#define VERIFY(c) do { ++checks; if (!(c)) { ++failed; fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); } } while(0)
static int on_drum(void*ctx,int kit,int v,float vel){(void)ctx;(void)kit;(void)v;(void)vel;return 1;}
static int on_bass(void*ctx,int b,int note,int accent,int slide,int velocity){(void)ctx;(void)b;(void)note;(void)accent;(void)slide;(void)velocity; return 1; }
static void off_bass(void*ctx,int b){(void)ctx;(void)b;}
static void on_brk(void*ctx,int st,int b,float per,int en){(void)ctx;(void)st;(void)b;(void)per;(void)en;}
static void off_brk(void*ctx){(void)ctx;}
static void out_midi(void*ctx,uint8_t s,uint8_t a,uint8_t b){(void)ctx;(void)s;(void)a;(void)b;}
static void on_step(void*ctx,int part,int step){(void)ctx;(void)step;if(part==TRK_BASS0)++played;}
static const seq_sink_t sink={on_drum,on_bass,off_bass,on_brk,off_brk,out_midi,on_step,0, 0};

static void setup(seq_t *s, int dir, int len)
{
    unsigned p;
    for(p=0;p<NPAT;p++) pattern_init(&pat[p],0x3B0u+p*977u,0x5A1u+p*613u);
    pat[0].bass[0].dir = (uint8_t)dir;
    pat[0].bass[0].len = (uint8_t)len;
    pat[0].bass[0].gen.mutate_bars = 1; /* mutation should track actual bars */
    seq_init(s,pat);
    s->send_clock = 0;
    played=0;
    seq_start(s);
    seq_advance(s,0,&sink);
    VERIFY(played==1);
    VERIFY(s->t[TRK_BASS0].bars==0);
}
static void next(seq_t *s)
{
    unsigned old=played;
    s->t[TRK_BASS0].to_next=0;  /* drive exactly one *real* production event */
    seq_advance(s,0,&sink);
    VERIFY(played==old+1);
    VERIFY(s->t[TRK_BASS0].pos<seq_part_len(s,TRK_BASS0));
}
static void run_direction(int dir, int len, int total)
{
    seq_t s;
    int event;
    unsigned period = (dir==DIR_PINGPONG && len>1) ? 2u*(unsigned)(len-1) : (unsigned)len;
    setup(&s,dir,len);
    for(event=1; event<=total; event++) {
        unsigned expected_bars=(unsigned)(event-1)/(period?period:1);
        VERIFY(s.t[TRK_BASS0].bars==expected_bars);
        if ((unsigned)event<=period) VERIFY(s.mut_rng[0]==0); /* no premature mutation */
        else VERIFY(s.mut_rng[0]!=0); /* TB-3PO mutate did execute at boundary */
        if(event<total) next(&s);
    }
    seq_stop(&s);
    seq_advance(&s,0,&sink);
    seq_start(&s);
    seq_advance(&s,0,&sink);
    VERIFY(s.t[TRK_BASS0].bars==0); /* fresh PLAY starts a fresh logical pass */
    VERIFY(s.t[TRK_BASS0].pos==(dir==DIR_REV?len-1:0));
}
static void resize_random(void)
{
    seq_t s;
    setup(&s,DIR_RANDOM,8);
    next(&s); next(&s); next(&s);
    VERIFY(s.t[TRK_BASS0].bars==0);
    pat[0].bass[0].len=2; /* musician shortens a playing 303 line */
    next(&s);
    VERIFY(s.t[TRK_BASS0].bars==1); /* do not get stuck until the former length */
    VERIFY(s.t[TRK_BASS0].pos<2);
}
static void switch_direction(void)
{
    seq_t s;
    setup(&s,DIR_RANDOM,4);
    next(&s); next(&s);
    {
        unsigned old_bars = s.t[TRK_BASS0].bars;
        int old_pos = s.t[TRK_BASS0].pos;
        pat[0].bass[0].dir=DIR_FWD;
        next(&s);
        VERIFY(s.t[TRK_BASS0].bars==old_bars+(unsigned)(old_pos==3));
        old_bars = s.t[TRK_BASS0].bars;
        pat[0].bass[0].dir=DIR_RANDOM;
        for (int i=1; i<=5; i++) {
            next(&s);
            VERIFY(s.t[TRK_BASS0].bars==old_bars+(unsigned)(i==5));
        }
    }
}
int main(void)
{
    VERIFY(sizeof(seq_track_t)==28); /* preserve tiny embedded per-track RAM allocation */
    run_direction(DIR_FWD,4,17);
    run_direction(DIR_REV,4,17);
    run_direction(DIR_PINGPONG,4,19);
    run_direction(DIR_RANDOM,4,17);
    run_direction(DIR_RANDOM,1,6);
    run_direction(DIR_RANDOM,64,131);
    resize_random();
    switch_direction();
    printf("303 direction logical-bar regression: %u/%u checks\n", checks-failed, checks);
    return failed ? 1 : 0;
}
