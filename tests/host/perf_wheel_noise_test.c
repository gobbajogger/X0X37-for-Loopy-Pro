/* SPDX-License-Identifier: GPL-3.0-only */
/* Owner 2026-10-31 (nightly 0.0.13): Chicago/Miami params jumped by themselves and
 * lanes recorded with the wheels untouched. A resting mod/pitch wheel with ADC jitter
 * and one spike must not write cutoff / wobble depth / bend or touch a motion lane.
 * A real move still works. */
#define CHICAGO_PRESENTATION_NO_MAIN
#include "chicago_presentation_test.c"
static void wheels(int pitch,int mod)
{fm1_in.frames+=400;fm1_in.wheel[0]=pitch;fm1_in.wheel[1]=mod;smk_wheels();audio();}
static void steps(int n) {for(int i=0;i<n*40;i++)audio();}
static int plane(int v,int param)
{int part=engine_motion_part(T_303,v);return motion_find(proj.arr.lane,seq.ppat[part],part,T_303,v,param);}
static void play(int on){seq.clk_src=SEQ_CLK_AUTO;if(on)seq_start(&seq);else seq_stop(&seq);steps(1);}
static const int J[8]={0,2,-1,3,-3,1,-2,0};
int main(void)
{
    boot();run_ms(50);engine_guard_enable(0);
    fixture(0,0);
    int mod=engine_bass_mod_param(0),bend=ORIGINAL_CHICAGO_PITCH_BEND;
    for(int i=0;i<8;i++)wheels(550,615);                 /* rest: calibrate */
    CHECK(engine_bass_perf(0,2,1)==-1);
    /* one ADC spike on each wheel, then rest with +-3 jitter, REC on and playing */
    wheels(550+40,615+30);wheels(550,615);
    ui.rec=1;play(1);
    for(int i=0;i<400;i++){wheels(550+J[i&7],615+J[(i+3)&7]);if(!(i&15))steps(1);}
    CHECK(engine_bass_perf(0,2,1)==-1);                  /* the wheel never took the cutoff */
    CHECK(engine_bass_perf(0,0,1)==64);                  /* bend stayed centred */
    CHECK(plane(0,mod)<0);CHECK(plane(0,bend)<0);        /* nothing recorded */
    ui.rec=0;play(0);steps(2);
    /* a real move still writes */
    wheels(550,485);CHECK(engine_bass_perf(0,2,1)>64);
    wheels(620,485);CHECK(engine_bass_perf(0,0,1)>64);
    wheels(550,615);CHECK(engine_bass_perf(0,0,1)==64);
    printf("PASS %u resting-wheel noise checks\n",checks);
    return 0;
}
