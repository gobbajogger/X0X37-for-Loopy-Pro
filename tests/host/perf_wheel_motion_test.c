/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago and Miami wheels as recordable performance (owner 2026-10-30):
 * pitch wheel / ext pitch bend = the engine's bend; mod wheel / ext CC1 on the
 * part's channel = Chicago cutoff, Miami wobble depth; damper stays on CC64.
 * With REC on and playing, a move records a motion lane that plays back. */
#define CHICAGO_PRESENTATION_NO_MAIN
#include "chicago_presentation_test.c"
static void midi(int ch,int st,int d1,int d2)
{min_q[mi_w++%MQ]=(uint32_t)((st>>4)|((st|ch)<<8)|(d1<<16)|(d2<<24));audio();ui_input_only();smk_wheels();}
static void wheels(int pitch,int mod)
{fm1_in.frames+=400;fm1_in.wheel[0]=pitch;fm1_in.wheel[1]=mod;smk_wheels();audio();}
static void steps(int n) {for(int i=0;i<n*40;i++)audio();}
static int plane(int v,int param)
{int part=engine_motion_part(T_303,v);return motion_find(proj.arr.lane,seq.ppat[part],part,T_303,v,param);}
static int recorded(int k,int lo)
{int r=0;if(k<0)return 0;for(int s=0;s<NSTEPS;s++)r|=proj.arr.lane[k].val[s]!=MOT_NONE && proj.arr.lane[k].val[s]>lo;return r;}
static void play(int on){seq.clk_src=SEQ_CLK_AUTO;if(on)seq_start(&seq);else seq_stop(&seq);steps(1);}
int main(void)
{
    boot();run_ms(50);engine_guard_enable(0);
    for(int v=0;v<2;v++)for(int bank=0;bank<2;bank++) {
        fixture(v,bank);
        int bend=(bank?ORIGINAL_MIAMI_PITCH_BEND:ORIGINAL_CHICAGO_PITCH_BEND),mod=engine_bass_mod_param(v),ch=seq.ch[TRK_BASS0+v];
        CHECK(mod==(bank?ORIGINAL_MIAMI_WOBBLE_DEPTH:ORIGINAL_CHICAGO_CUTOFF));CHECK(ch<16);
        /* live: external MIDI on the part's channel */
        midi(ch,0xB0,1,120);CHECK(engine_bass_perf(v,2,1)==120 && engine_bass_perf(v,2,0)==120);
        midi(ch,0xB0,64,127);CHECK(engine_bass_perf(v,1,1)==127);  /* damper: CC64 */
        midi(ch,0xB0,64,0);CHECK(engine_bass_perf(v,1,1)==0);
        midi(ch,0xE0,0,127);CHECK(engine_bass_perf(v,0,1)==127);
        midi(ch,0xE0,0,64);CHECK(engine_bass_perf(v,0,1)==64);
        /* live: device wheels (rest first: self-calibrated centre) */
        for(int i=0;i<4;i++)wheels(550,615);
        wheels(600,615);CHECK(engine_bass_perf(v,0,1)>64);
        wheels(550,485);CHECK(engine_bass_perf(v,2,1)>64);
        wheels(550,615);CHECK(engine_bass_perf(v,0,1)==64);
        CHECK(plane(v,bend)<0 && plane(v,mod)<0);            /* not playing: no lane */
        /* REC on + playing: the device wheels record */
        ui.rec=1;play(1);
        for(int i=0;i<6;i++){wheels(i&1?610:600,i&1?480:490);steps(1);}
        wheels(550,615);steps(1);
        CHECK(recorded(plane(v,bend),64));CHECK(recorded(plane(v,mod),64));
        /* playback with REC off and wheels at rest */
        ui.rec=0;
        int pb=0,pm=0;for(int i=0;i<40*40;i++){audio();pb|=engine_bass_perf(v,0,0)>64;pm|=engine_bass_perf(v,2,0)>64;}
        CHECK(pb && pm);
        play(0);steps(2);
        /* external MIDI records too */
        memset(proj.arr.lane,0,sizeof proj.arr.lane);ui.rec=1;play(1);
        for(int i=0;i<6;i++){midi(ch,0xE0,0,120);midi(ch,0xB0,1,110+i);steps(1);}
        CHECK(recorded(plane(v,bend),64));CHECK(recorded(plane(v,mod),100));
        midi(ch,0xE0,0,64);ui.rec=0;play(0);steps(2);
        memset(proj.arr.lane,0,sizeof proj.arr.lane);
    }
    printf("PASS %u wheel/ext-MIDI bend and mod checks (live, REC record, playback)\n",checks);
    return 0;
}
