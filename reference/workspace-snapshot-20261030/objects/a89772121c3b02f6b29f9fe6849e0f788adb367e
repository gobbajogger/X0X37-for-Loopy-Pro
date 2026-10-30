/* SPDX-License-Identifier: GPL-3.0-only */
/* Real production sequencer clock-source handoff regression. */
#include <stdint.h>
#include <stdio.h>
#include "sequencer.h"

static pattern_t patterns[NPAT];
static unsigned bass_fired, checked, errors, mismatch_observed;
static int drum(void *c,int k,int v,float vel){(void)c;(void)k;(void)v;(void)vel;return 1;}
static int bass_on(void*c,int b,int n,int a,int s,int v){(void)c;(void)b;(void)n;(void)a;(void)s;(void)v; return 1; }
static void bass_off(void*c,int b){(void)c;(void)b;}
static void brk(void*c,int p,int b,float sp,int en){(void)c;(void)p;(void)b;(void)sp;(void)en;}
static void brk_stop(void*c){(void)c;}
static void midi(void*c,uint8_t s,uint8_t n,uint8_t v){(void)c;(void)s;(void)n;(void)v;}
static void step(void*c,int t,int p){(void)c;(void)p;if(t==TRK_BASS0)bass_fired++;}
static const seq_sink_t sink={drum,bass_on,bass_off,brk,brk_stop,midi,step,0};
static void check(int condition, const char*what,int dir,int len,int sw,int stage,uint32_t got,uint32_t want){
    checked++;
    if(!condition){errors++; if(errors<=16)fprintf(stderr,"FAIL %s dir=%d len=%d swing=%d stage=%d got=%u want=%u\n",what,dir,len,sw,stage,got,want);}
}
/* Each case uses actual Start, incoming MIDI clock ticks, scheduled note events,
 * and the audio-block application of the user's EXTERNAL->INTERNAL setting. */
static void run_case(int dir,int len,int sw,int stage){
    seq_t s;
    for(int i=0;i<NPAT;i++)pattern_init(&patterns[i],0x111111u+(unsigned)i,0x222222u+(unsigned)i);
    patterns[0].swing=(uint8_t)sw;
    patterns[0].bass[0].dir=(uint8_t)dir;
    patterns[0].bass[0].len=(uint8_t)len;
    seq_init(&s,patterns);
    s.clk_src=SEQ_CLK_EXT;
    s.send_clock=0;
    bass_fired=0;
    seq_start(&s);
    seq_advance(&s,0,&sink);
    seq_ext_clock(&s);
    seq_advance(&s,0,&sink);
    check(bass_fired==1,"first external step",dir,len,sw,stage,bass_fired,1);
    for(int pass=0;pass<stage;pass++){
        unsigned prev=bass_fired;
        for(int tick=0;tick<6;tick++){
            seq_ext_clock(&s);
            seq_advance(&s,0,&sink);
        }
        /* A swung step is held until its intentional audio-sample delay ends. */
        uint32_t next=s.t[TRK_BASS0].to_next;
        if(next!=0xFFFFFFFFu && next>255u)
            seq_advance(&s,(next+255u)>>8,&sink);
        check(bass_fired==prev+1,"next externally clocked step",dir,len,sw,stage,bass_fired,prev+1);
    }
    const seq_track_t *tr=&s.t[TRK_BASS0];
    check(s.ext==1,"before follows external",dir,len,sw,stage,s.ext,1);
    check(tr->to_next==0xFFFFFFFFu,"next step waits for tick",dir,len,sw,stage,tr->to_next,0xFFFFFFFFu);
    uint32_t per=s.ext_period?s.ext_period:seq_step_q8(&s,RATE_16)/6u;
    uint32_t since=s.ext_since<(1u<<23)?s.ext_since<<8:0xFFFFFFFFu;
    uint32_t left=(tr->ext_ticks==0xFFu?1u:6u-tr->ext_ticks)*per;
    /* RANDOM's high bit is the musical next time slot, not next random note position. */
    int next_is_odd=dir==DIR_RANDOM?((tr->mut_steps & 0x80u)!=0):(((tr->pos+1)%len)&1);
    uint32_t swq=(seq_step_q8(&s,RATE_16)/6u)*6u/200u*(unsigned)sw;
    /* Fixed external period drives precisely the same Q8 swing expression as switch_internal. */
    swq=(per*6u)/200u*(unsigned)sw;
    if(next_is_odd)left+=swq;
    uint32_t expected=left>since?left-since:0;
    int old_pos=tr->pos;
    uint8_t old_mut=tr->mut_steps;
    s.clk_src=SEQ_CLK_INT;
    seq_advance(&s,0,&sink);
    check(s.ext==0&&s.clk_cur==SEQ_CLK_INT,"now internal",dir,len,sw,stage,s.ext,0);
    check(s.t[TRK_BASS0].pos==old_pos&&s.t[TRK_BASS0].mut_steps==old_mut,"musical position preserved",dir,len,sw,stage,s.t[TRK_BASS0].pos,old_pos);
    uint32_t got=s.t[TRK_BASS0].to_next;
    if(got!=expected)mismatch_observed++;
    check(got==expected,"clock-handoff next-step phase",dir,len,sw,stage,got,expected);
    /* Observe the actual sequencer firing, not only its timer field. */
    unsigned prev_fire=bass_fired;
    if (expected > 512u) {
        uint32_t early=(expected>>8)-1u;
        seq_advance(&s,early,&sink);
        check(bass_fired==prev_fire,"no early internal note",dir,len,sw,stage,bass_fired,prev_fire);
    }
    uint32_t remain=s.t[TRK_BASS0].to_next;
    if (remain != 0xFFFFFFFFu)
        seq_advance(&s,(remain+255u)>>8,&sink);
    check(bass_fired==prev_fire+1u,"one internal note at next slot",dir,len,sw,stage,bass_fired,prev_fire+1u);
    seq_stop(&s);
    seq_advance(&s,0,&sink);
}
int main(void){
    for(int dir=DIR_FWD;dir<=DIR_RANDOM;dir+=DIR_RANDOM-DIR_FWD)
        for(int len=1;len<=4;len++)
            for(int sw=0;sw<=100;sw+=100)
                for(int stage=0;stage<8;stage++)run_case(dir,len,sw,stage);
    printf("clock-source-random-swing-handoff: %u/%u correct; mismatches=%u\n",checked-errors,checked,mismatch_observed);
    return errors?1:0;
}
