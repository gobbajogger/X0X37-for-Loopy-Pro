/* SPDX-License-Identifier: GPL-3.0-only */
/* REAL sequencer playback checks elapsed-step swing under random 303 direction. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "sequencer.h"

static pattern_t patterns[NPAT];
static unsigned bass_fired;
static int drum(void *c,int kit,int v,float vel) {(void)c;(void)kit;(void)v;(void)vel;return 1;}
static int bass_on(void*c,int b,int n,int a,int s,int v){(void)c;(void)b;(void)n;(void)a;(void)s;(void)v; return 1; }
static void bass_off(void*c,int b){(void)c;(void)b;}
static void brk(void*c,int p,int b,float sp,int en){(void)c;(void)p;(void)b;(void)sp;(void)en;}
static void brk_stop(void*c){(void)c;}
static void midi(void*c,uint8_t s,uint8_t n,uint8_t v){(void)c;(void)s;(void)n;(void)v;}
static void step(void*c,int part,int index){(void)c;(void)index; if (part==TRK_BASS0) bass_fired++;}
static const seq_sink_t sink={drum,bass_on,bass_off,brk,brk_stop,midi,step,0, 0};
static unsigned checks, bad;
static void verify(int b,const char *name,int dir,int idx,int pos,uint32_t got,uint32_t expected){
    checks++; if(!b){bad++; if(bad<20)fprintf(stderr,"FAIL %s dir=%d step=%d pos=%d intervalQ8=%u expected=%u\n",name,dir,idx,pos,got,expected);}
}
static void run(int dir,int len,int swing,unsigned events){
    seq_t s;
    for(int i=0;i<NPAT;i++)pattern_init(&patterns[i],0x3157u+i,0x7531u+i);
    patterns[0].swing=(uint8_t)swing;
    patterns[0].bass[0].dir=(uint8_t)dir;
    patterns[0].bass[0].len=(uint8_t)len;
    seq_init(&s,patterns);
    s.send_clock=0;
    bass_fired=0;
    seq_start(&s);
    seq_advance(&s,0,&sink);
    uint32_t L=seq_step_q8(&s,RATE_16), sw=L/200u*(uint32_t)swing;
    verify(bass_fired==1,"initial",dir,0,s.t[TRK_BASS0].pos,bass_fired,1);
    for(unsigned i=0;i<events;i++){
        const seq_track_t *t=&s.t[TRK_BASS0];
        uint32_t want=(i%2u==0?L+sw:L-sw);
        /* At 0 swing, this is always L. For swung random playback,
           musical time alternates independent of which step index was picked. */
        verify(t->to_next==want,"slot-swing",dir,(int)i,t->pos,t->to_next,want);
        if(i+1u==events)break;
        uint32_t k=(t->to_next+255u)>>8;
        unsigned old=bass_fired;
        seq_advance(&s,k,&sink); /* actual sequencer consumes the elapsed audio samples */
        verify(bass_fired==old+1u,"one-bass-step",dir,(int)i,t->pos,bass_fired,old+1u);
    }
    seq_stop(&s);
    seq_advance(&s,0,&sink);
}
static void run_external_random(int len,int swing,unsigned events){
    seq_t s;
    for(int i=0;i<NPAT;i++)pattern_init(&patterns[i],0x3157u+i,0x7531u+i);
    patterns[0].swing=(uint8_t)swing;
    patterns[0].bass[0].dir=DIR_RANDOM;
    patterns[0].bass[0].len=(uint8_t)len;
    seq_init(&s,patterns);
    s.clk_src=SEQ_CLK_EXT;
    s.send_clock=0;
    bass_fired=0;
    seq_start(&s);
    seq_advance(&s,0,&sink);
    verify(bass_fired==0,"ext-pending",DIR_RANDOM,-1,0,bass_fired,0);
    seq_ext_clock(&s);
    seq_advance(&s,0,&sink);
    verify(bass_fired==1,"ext-first",DIR_RANDOM,0,s.t[TRK_BASS0].pos,bass_fired,1);
    for(unsigned i=1;i<events;i++){
        unsigned before=bass_fired;
        for(int tick=0;tick<6;tick++){
            seq_ext_clock(&s);
            seq_advance(&s,0,&sink);
        }
        uint32_t L=s.ext_period?s.ext_period*6u:seq_step_q8(&s,RATE_16);
        uint32_t sw=L/200u*(uint32_t)swing;
        uint32_t expected=(i&1u)?sw:0;
        uint32_t pending=s.t[TRK_BASS0].to_next;
        /* With zero swing, the expected step is already played in this block. */
        if(expected)
            verify(pending==expected,"ext-swing-offset",DIR_RANDOM,(int)i,s.t[TRK_BASS0].pos,pending,expected);
        else
            verify(bass_fired==before+1u,"ext-straight-step",DIR_RANDOM,(int)i,s.t[TRK_BASS0].pos,bass_fired,before+1u);
        if(pending>255u)
            seq_advance(&s,(pending+255u)>>8,&sink);
        verify(bass_fired==before+1u,"ext-one-step",DIR_RANDOM,(int)i,s.t[TRK_BASS0].pos,bass_fired,before+1u);
    }
    seq_stop(&s);
    seq_advance(&s,0,&sink);
}
int main(void){
    run(DIR_RANDOM,4,100,160);
    run(DIR_RANDOM,3,100,99);
    run(DIR_RANDOM,1,100,32);
    run(DIR_RANDOM,4,0,64);
    run(DIR_FWD,4,100,64);
    run_external_random(4,100,64);
    run_external_random(3,100,64);
    run_external_random(4,0,64);
    printf("random-clock-slot-swing: %u/%u\n",checks-bad,checks);
    return bad?1:0;
}
