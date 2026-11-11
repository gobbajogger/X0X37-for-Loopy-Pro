/* SPDX-License-Identifier: GPL-3.0-only */
/* M13: real-linked sequencer regression. Catches rate-index masking, wrong
 * subdivisions, per-part aliasing, early NoteOff and accepted-clock drift. */
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"
static pattern_t patterns[NPAT];
static seq_t seq;
static unsigned now, checks, failed;
static unsigned steps[NTRACKS], on[NBASS], off[NBASS], midi_off[NBASS];
static unsigned first_step[NTRACKS], second_step[NTRACKS], last_step[NTRACKS];
static uint32_t trace=2166136261u;
static void record(unsigned kind,unsigned part,unsigned value){
    const unsigned fields[]={now,kind,part,value};
    for(unsigned i=0;i<4;i++){trace^=fields[i];trace*=16777619u;}
}
static const unsigned durations[6]={4410,2940,2205,5880,8820,17640};
static const unsigned ticks[6]={6,4,3,8,12,24};
#define CHECK(x) do { checks++; if(!(x)){failed++; if(failed<30) fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);}}while(0)
static void step(void *ctx,int part,int pos) {
    (void)ctx;record(0,(unsigned)part,(unsigned)pos);
    if(steps[part]==0)first_step[part]=now;
    if(steps[part]==1)second_step[part]=now;
    last_step[part]=now;steps[part]++;
}
static int note_on(void *ctx,int part,int note,int accent,int slide,int velocity) {
    (void)ctx;record(1,(unsigned)part,(unsigned)note);record(2,(unsigned)accent,(unsigned)(slide*128+velocity));on[part]++;return 1;
}
static void note_off(void *ctx,int part){(void)ctx;record(3,(unsigned)part,0);off[part]++;}
static void midi(void *ctx,uint8_t st,uint8_t note,uint8_t velocity){
    (void)ctx;record(4,st,(unsigned)(note*128+velocity));
    if(st==0x81)midi_off[0]++;
    if(st==0x82)midi_off[1]++;
}
static const seq_sink_t sink={.step=step,.bass_on=note_on,.bass_off=note_off,.midi=midi};
static void setup(void){
    for(int p=0;p<NPAT;p++)pattern_init(&patterns[p],123,456);
    seq_init(&seq,patterns);seq.bpm=150;seq.send_clock=0;seq.send_notes=1;now=0;
    for(int p=0;p<NTRACKS;p++)steps[p]=first_step[p]=second_step[p]=last_step[p]=0;
    for(int b=0;b<NBASS;b++)on[b]=off[b]=midi_off[b]=0;
}
static void run(unsigned n){
    seq_advance(&seq,0,&sink);
    while(n){unsigned k=seq_until_event(&seq,n>128?128:n);now+=k;seq_advance(&seq,k,&sink);n-=k;}
}
static void rates(int a,int b,int c,int d){
    patterns[0].drum[0].rate=(uint8_t)a;patterns[0].drum[1].rate=(uint8_t)b;
    patterns[0].bass[0].rate=(uint8_t)c;patterns[0].bass[1].rate=(uint8_t)d;
}
static void duration_lookup(void){
    setup();
    /* Published values retain exact sample durations; appended 4/5 must not
     * alias0/1. At150 BPM one quarter is17640 samples. */
    for(int r=0;r<6;r++)CHECK(seq_step_q8(&seq,r)==durations[r]*256u);
    const int bad[]={-1,INT_MIN,6,7,255,INT_MAX};
    for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++)CHECK(seq_step_q8(&seq,bad[i])==4410u*256u);
    for(int r=6;r<=255;r++)CHECK(seq_step_q8(&seq,r)==4410u*256u);
    seq.bpm=20;CHECK(seq_step_q8(&seq,4)==66150u*256u);CHECK(seq_step_q8(&seq,5)==132300u*256u);
    seq.bpm=300;CHECK(seq_step_q8(&seq,4)==4410u*256u);CHECK(seq_step_q8(&seq,5)==8820u*256u);
}
static void cadence(void){
    for(int ext=0;ext<2;ext++)for(int r=0;r<6;r++){
        setup();rates(r,r,r,r);
        if(ext)seq.clk_src=SEQ_CLK_EXT;
        seq_start(&seq);
        if(ext)for(unsigned t=0;t<=ticks[r];t++){seq_ext_clock(&seq);run(735);}
        else run(durations[r]+1);
        for(int p=0;p<4;p++){CHECK(steps[p]==2);CHECK(first_step[p]==0);CHECK(second_step[p]==durations[r]);}
        /* BREAK remains16ths in this core-only stage. */
        if(durations[r]>=4410) CHECK(second_step[TRK_BRK]==4410);
        else CHECK(steps[TRK_BRK]==1);
    }
    setup();rates(4,5,0,3);seq_start(&seq);run(17641);
    CHECK(steps[0]==3);CHECK(steps[1]==2);CHECK(steps[2]==5);CHECK(steps[3]==4);
    /* Malformed live bytes must not index outside the table or wrap to1/32. */
    setup();rates(254,255,6,7);seq_start(&seq);run(4411);
    for(int p=0;p<4;p++)CHECK(steps[p]==2 && second_step[p]==4410);
}
static void held_notes_and_stop(void){
    for(int ext=0;ext<2;ext++)for(int r=0;r<6;r++){
        setup();rates(r,r,r,r);
        for(int b=0;b<NBASS;b++){
            patterns[0].bass[b].step[0]=(bstep_t){36,G_NOTE,100};
            patterns[0].bass[b].step[1]=(bstep_t){99,G_TIE,42};
            patterns[0].bass[b].step[2]=(bstep_t){99,G_TIE,42};
        }
        if(ext)seq.clk_src=SEQ_CLK_EXT;
        seq_start(&seq);
        if(ext)for(unsigned t=0;t<ticks[r]*2;t++){seq_ext_clock(&seq);run(735);}
        else run(durations[r]*2-1);
        for(int b=0;b<NBASS;b++)CHECK(on[b]==1 && off[b]==0 && midi_off[b]==0);
        seq_stop(&seq);run(1);
        for(int b=0;b<NBASS;b++){CHECK(off[b]==1 && midi_off[b]==1);CHECK(!seq.t[TRK_BASS0+b].sounding && !seq.bass_out_valid[b]);}
        CHECK(!seq.playing);
        unsigned oldsteps=steps[0];
        if(ext)for(unsigned t=0;t<ticks[r]*2;t++){seq_ext_clock(&seq);run(735);}
        else run(durations[r]*2);
        CHECK(steps[0]==oldsteps);
        seq_continue(&seq);run(1);
        if(ext){seq_ext_clock(&seq);run(735);}
        CHECK(seq.playing);
    }
}
/* Clock-source handoff must preserve the next musical slot at new rates. */
static void clock_handoff(void){
    for(int r=4;r<6;r++){
        setup();rates(r,r,r,r);seq.clk_src=SEQ_CLK_EXT;seq_start(&seq);
        for(unsigned t=0;t<=ticks[r];t++){seq_ext_clock(&seq);run(735);}
        CHECK(steps[0]==2);
        seq.clk_src=SEQ_CLK_INT;run(durations[r]-735);
        for(int p=0;p<4;p++){CHECK(steps[p]==3);CHECK(last_step[p]==durations[r]*2);}
        unsigned before=steps[0];seq_ext_clock(&seq);run(1);
        CHECK(steps[0]==before); /* INTERNAL rejects foreign pending ticks. */
    }
}
/* Differential mode deliberately uses only published numeric values0..3;
 * run the same test against frozen base and candidate and compare its trace. */
static void legacy_trace(void){
    const unsigned tempos[]={20,125,150,275,300};
    for(unsigned tempo=0;tempo<5;tempo++)for(int r=0;r<4;r++)for(int dir=0;dir<4;dir++){
        setup();seq.bpm=(float)tempos[tempo];rates(r,(r+1)%4,(r+2)%4,(r+3)%4);
        patterns[0].swing=(uint8_t)(dir%3*50);
        for(int b=0;b<NBASS;b++){
            patterns[0].bass[b].dir=(uint8_t)dir;
            patterns[0].bass[b].step[0]=(bstep_t){36,G_NOTE|BS_SLIDE,100};
            patterns[0].bass[b].step[1]=(bstep_t){43,G_NOTE,64};
            patterns[0].bass[b].step[2]=(bstep_t){99,G_TIE,42};
        }
        seq_start(&seq);run(88200);seq_stop(&seq);run(1);seq_continue(&seq);run(22050);
    }
    printf("legacy rate trace: %08x\n",trace);
}
int main(int argc,char **argv){if(argc==2&&!strcmp(argv[1],"--legacy-trace")){legacy_trace();return 0;}duration_lookup();cadence();held_notes_and_stop();clock_handoff();printf("SEQ eighth/quarter: %u/%u checks passed\n",checks-failed,checks);return failed!=0;}

