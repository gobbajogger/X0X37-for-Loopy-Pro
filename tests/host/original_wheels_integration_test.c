/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual production scanner -> command queue -> original engines. The only
 * substitutes are the host platform and an explicit audio-preemption hook.
 * No ADC/device behavior or hardware timing is claimed. */
#define main simulator_main
#include WHEEL_HOST_SOURCE
#undef main
#include "app/original_sound.h"

static struct { uint32_t frames; int32_t wheel[2]; } fm1_in;
/* Exact native utility from firmware/src/x0x.c, absent in the host unity. */
static inline int32_t clamp(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }
#include "smk_wheels.inc"

extern unsigned wheel_probe_queue(void);
extern unsigned wheel_probe_owners(void);
extern unsigned wheel_probe_revision(int);
extern void wheel_probe_motion_step(int,int,int);
extern void wheel_probe_motion_tick(unsigned);
extern void wheel_probe_motion_release(void);
extern uint32_t wheel_probe_native105_bits(int);
extern uint32_t wheel_probe_bipolar_bits(int);
static unsigned checks, preempt_once, preempted;
static int preempt_voice, preempt_bank;
static uint64_t pcm_hash = UINT64_C(1469598103934665603);
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static uint32_t bits(float f) { uint32_t u; memcpy(&u,&f,4); return u; }
static void audio(unsigned frames)
{
    int32_t out[514];
    for(unsigned i=0;i<514;i++)out[i]=INT32_C(0x12345678);
    engine_render(out,frames);
    CHECK(out[frames*2]==INT32_C(0x12345678));
    CHECK(out[frames*2+1]==INT32_C(0x12345678));
    for(unsigned i=0;i<frames*2;i++){
        uint32_t u=(uint32_t)out[i];
        for(unsigned j=0;j<4;j++)pcm_hash=(pcm_hash^(uint8_t)(u>>(8*j)))*UINT64_C(1099511628211);
    }
}
void wheel_probe_bank_query_hook(void)
{
    if(!preempt_once)return;
    preempt_once=0;preempted++;
    audio(1); /* Audio preemption after bank selection, before owner publication. */
    CHECK(engine_bass_bank(preempt_voice)==preempt_bank);
}
static void settle(void){for(unsigned n=0;n<12;n++)audio(256);}
static void physical(int pitch,int mod)
{fm1_in.wheel[0]=pitch;fm1_in.wheel[1]=mod;smk_wheels();}
static void midi(unsigned source,unsigned ch,unsigned type,unsigned d1,unsigned d2)
{unsigned st=type|ch;min_q[mi_w++%MQ]=(st>>4)|(source<<4)|(st<<8)|(d1<<16)|(d2<<24);}
static void midi_bend(unsigned source,unsigned ch,unsigned value)
{midi(source,ch,0xe0,value&127u,value>>7);}
static void select_kind(int voice,unsigned kind)
{
    CHECK(engine_bass_select_preset(voice,(int)kind,0,&proj.sound)==0);
    settle();
    CHECK(engine_bass_preset_pending(voice)==0);
    CHECK(engine_bass_bank(voice)==(int)kind);
    CHECK(engine_bass_status(voice)==1);
}
static void cold(void)
{
    engine_project_hold(1);
    engine_init(proj.pat,&proj.arr.song,proj.arr.lane,&proj.sound);
    engine_apply_sound(&proj.sound);
    seq.ch[PART_909]=9;seq.ch[PART_808]=10;seq.ch[PART_BRK]=11;
    seq.ch[PART_303A]=2;seq.ch[PART_303B]=3;
    engine_project_release();settle();
}
static void macro_is(int voice,unsigned p,float f)
{CHECK(engine_original_value_bits(voice,p)==bits(f));CHECK(engine_bass_status(voice)==1);}
static void bend_is(int voice,unsigned p,float macro,float native,float bipolar)
{
    macro_is(voice,p,macro);
    CHECK(wheel_probe_native105_bits(voice)==bits(native));
    CHECK(wheel_probe_bipolar_bits(voice)==bits(bipolar));
}
int main(int argc,char **argv)
{
    CHECK(argc==3);
    int voice=atoi(argv[1]),kind=atoi(argv[2]),other=1-voice;
    CHECK((unsigned)voice<2 && (unsigned)kind<2);
    unsigned pb=kind?16u:20u,mod=kind?12u:2u,other_pb=kind?20u:16u,other_mod=kind?2u:12u;
    load_force=0;boot();settle();
    select_kind(voice,(unsigned)kind);select_kind(other,(unsigned)(1-kind));cold();
    ui.view=V_PART;ui.part=(uint8_t)(PART_303A+voice);
    fm1_in.frames=400;physical(550,615);
    CHECK(wheel_probe_queue()==0 && wheel_probe_owners()==0);
    bend_is(voice,pb,.5f,.5f,0.0f);
    sound_t saved=proj.sound;

    /* Real physical key ownership survives both bend extremes and center. */
    uint32_t generation,context;int arp;
    engine_live_key_state(voice,&generation,&arp,&context);
    CHECK(engine_live_key(voice,generation,1,60,100,0,0));audio(128);
    CHECK(engine_original_keys(voice,0)==1);
    physical(630,615);CHECK(wheel_probe_queue()==1);
    macro_is(voice,pb,.5f); /* Main loop only queued the edit. */
    audio(128);bend_is(voice,pb,1.0f,1.0f,1.0f);
    macro_is(other,other_pb,.5f);
    CHECK(wheel_probe_owners()==(1u<<voice));
    ui.part=(uint8_t)(PART_303A+other);
    unsigned revision=wheel_probe_revision(voice);
    for(unsigned i=0;i<8;i++)physical(630,615);
    CHECK(wheel_probe_queue()==0 && wheel_probe_revision(voice)==revision);
    physical(470,615);audio(128);bend_is(voice,pb,0.0f,0.0f,-1.0f);
    CHECK(engine_original_keys(voice,0)==1);
    physical(550,615);CHECK(wheel_probe_queue()==1);audio(128);
    bend_is(voice,pb,.5f,.5f,0.0f);
    CHECK(wheel_probe_owners()==0 && engine_original_keys(voice,0)==1);
    CHECK(!memcmp(&saved,&proj.sound,sizeof saved));
    CHECK(engine_live_key(voice,generation,1,60,0,0,0));audio(128);
    CHECK(engine_original_keys(voice,0)==0);
    cold(); /* Independent subsequent scenarios start with no release tail. */

    /* Mod is the engine-specific ephemeral control and retains its part. */
    ui.part=(uint8_t)(PART_303A+voice);
    saved=proj.sound;
    physical(550,485);audio(1);macro_is(voice,mod,1.0f);
    ui.part=(uint8_t)(PART_303A+other);
    revision=wheel_probe_revision(voice);
    physical(550,485);CHECK(wheel_probe_queue()==0 && wheel_probe_revision(voice)==revision);
    physical(550,550);audio(1);macro_is(voice,mod,57.0f/127.0f);
    physical(550,615);audio(1);macro_is(voice,mod,0.0f);
    CHECK(wheel_probe_owners()==0 && !memcmp(&saved,&proj.sound,sizeof saved));

    /* Each wheel can retain a different part through unrelated UI navigation. */
    ui.part=(uint8_t)(PART_303A+voice);physical(630,615);audio(1);
    ui.part=(uint8_t)(PART_303A+other);physical(630,485);audio(1);
    CHECK(wheel_probe_owners()==3);
    macro_is(voice,pb,1.0f);macro_is(other,other_mod,1.0f);
    ui.part=PART_BRK;physical(470,550);audio(1);
    macro_is(voice,pb,0.0f);macro_is(other,other_mod,57.0f/127.0f);
    physical(550,550);audio(1);CHECK(wheel_probe_owners()==(1u<<other));
    physical(550,615);audio(1);CHECK(wheel_probe_owners()==0);
    CHECK(!memcmp(&saved,&proj.sound,sizeof saved));
    cold();

    /* Exercise actual publication hold and actual cold adoption, not stubs. */
    ui.part=(uint8_t)(PART_303A+voice);
    CHECK(engine_bass_select_preset(voice,1-kind,0,&proj.sound)==0);
    preempt_voice=voice;preempt_bank=kind;preempt_once=1;
    physical(630,615);CHECK(preempted==1 && !preempt_once);
    audio(1);CHECK(engine_bass_bank(voice)==kind && engine_bass_preset_pending(voice)==3);
    macro_is(voice,pb,1.0f);
    physical(550,615);audio(1);
    CHECK(engine_bass_preset_pending(voice)==0 && engine_bass_bank(voice)==1-kind);
    bend_is(voice,other_pb,.5f,.5f,0.0f);
    select_kind(voice,(unsigned)kind);cold();

    /* Real MIDI admission, all three sources, LSB witnesses and exact center. */
    static const unsigned values[]={0,1,127,128,8191,8192,8193,16382,16383};
    saved=proj.sound;
    for(unsigned source=0;source<3;source++)for(unsigned i=0;i<sizeof values/sizeof values[0];i++){
        unsigned u=values[i];
        float expected=u<8192?(float)u/16384.0f:.5f+(float)(u-8192)/16382.0f;
        midi_bend(source,(unsigned)(2+voice),u);audio(1);
        macro_is(voice,pb,expected);
        macro_is(other,other_pb,.5f);
    }
    midi_bend(0,(unsigned)(2+voice),8192);audio(1);bend_is(voice,pb,.5f,.5f,0.0f);
    revision=wheel_probe_revision(voice);
    midi_bend(0,15,0);audio(1);CHECK(wheel_probe_revision(voice)==revision);
    unsigned mod_bits=engine_original_value_bits(voice,mod);
    midi(0,(unsigned)(2+voice),0xb0,1,127);audio(1);
    CHECK(engine_original_value_bits(voice,mod)==mod_bits); /* CC1 currently unhandled. */
    for(unsigned d=0;d<2;d++){
        midi(0,(unsigned)(2+voice),0xb0,64,d?127:0);audio(1);
        macro_is(voice,pb+1,d?1.0f:0.0f);
    }
    midi(0,(unsigned)(2+voice),0xb0,64,0);audio(1);
    CHECK(!memcmp(&saved,&proj.sound,sizeof saved));

    /* A physical hold is part-wide; a later MIDI bend wins until another move. */
    ui.part=(uint8_t)(PART_303A+voice);physical(630,615);audio(1);
    midi_bend(1,(unsigned)(2+voice),0);audio(1);macro_is(voice,pb,0.0f);
    physical(630,615);audio(1);macro_is(voice,pb,0.0f);
    physical(550,615);audio(1);bend_is(voice,pb,.5f,.5f,0.0f);

    /* Performance controls cannot become saved patch edits, even through the
     * real UI preference setter. Ordinary controls remain persistent. */
    saved=proj.sound;
    CHECK(wheel_probe_queue()==0);
    pref_set(PR(R_ENG,T_303,voice,pb),20);
    pref_set(PR(R_ENG,T_303,voice,pb+1),127);
    CHECK(wheel_probe_queue()==0 && !memcmp(&saved,&proj.sound,sizeof saved));
    audio(1);bend_is(voice,pb,.5f,.5f,0.0f);macro_is(voice,pb+1,0.0f);
    pref_set(PR(R_ENG,T_303,voice,1),41);audio(1);
    macro_is(voice,1,41.0f/127.0f);

    /* Load valid historical schema2 bytes, including two stale performance
     * edits beside the ordinary edit. Never rewrite that stored payload. */
    uint8_t *cell=proj.sound.v[T_303][voice];
    cell[pb]=20;cell[pb+1]=127;
    uint32_t mask=original_sound_edit_mask(cell)|(UINT32_C(3)<<pb);
    for(unsigned j=0;j<4;j++)cell[22+j]=(uint8_t)(mask>>(8*j));
    CHECK(original_sound_valid(cell));
    engine_bass_sound_refresh(&proj.sound,voice);
    CHECK(engine_bass_sound_validate(&proj.sound)==0);
    saved=proj.sound;cold();
    bend_is(voice,pb,.5f,.5f,0.0f);macro_is(voice,pb+1,0.0f);
    macro_is(voice,1,41.0f/127.0f);
    CHECK(!memcmp(&saved,&proj.sound,sizeof saved));
    physical(630,615);audio(1);bend_is(voice,pb,1.0f,1.0f,1.0f);
    physical(550,615);audio(1);bend_is(voice,pb,.5f,.5f,0.0f);
    midi(0,(unsigned)(2+voice),0xb0,64,127);audio(1);macro_is(voice,pb+1,1.0f);
    midi(0,(unsigned)(2+voice),0xb0,64,0);audio(1);macro_is(voice,pb+1,0.0f);
    CHECK(!memcmp(&saved,&proj.sound,sizeof saved));

    /* Exercise actual motion step/ramp/release callbacks with stale saved
     * performance lanes. Their scheduler still runs, but cannot touch live
     * wheels/pedal or turn the retained old patch byte into a bend bias. */
    CHECK(motion_count(proj.arr.lane)==0);
    int part=PART_303A+voice,pat=seq.ppat[part];
    int pbl=motion_alloc(proj.arr.lane,pat,part,T_303,voice,(int)pb);
    int pedal=motion_alloc(proj.arr.lane,pat,part,T_303,voice,(int)pb+1);
    CHECK(pbl>=0 && pedal>=0 && pbl!=pedal);
    proj.arr.lane[pbl].val[0]=0;proj.arr.lane[pbl].val[1]=127;
    proj.arr.lane[pedal].val[0]=0;proj.arr.lane[pedal].val[1]=127;
    /* Retain identity gating: Miami16 must not become Chicago ARP speed. */
    CHECK(engine_bass_select_preset(voice,1-kind,0,&proj.sound)==-4);
    int ordinary=motion_alloc(proj.arr.lane,pat,part,T_303,voice,1);
    int otherlane=motion_alloc(proj.arr.lane,seq.ppat[PART_303A+other],PART_303A+other,T_303,other,1);
    CHECK(ordinary>=0 && otherlane>=0);
    proj.arr.lane[ordinary].val[0]=100;proj.arr.lane[ordinary].val[1]=0;
    proj.arr.lane[otherlane].val[0]=55;
    lane_t other_saved=proj.arr.lane[otherlane];
    physical(630,615);audio(1);
    midi(0,(unsigned)(2+voice),0xb0,64,127);audio(1);
    wheel_probe_motion_step(voice,pat,0);
    CHECK(engine_motion_value(pbl)==0 && engine_motion_value(pedal)==0);
    macro_is(voice,pb,1.0f);macro_is(voice,pb+1,1.0f);macro_is(voice,1,100.0f/127.0f);
    wheel_probe_motion_tick(64);
    wheel_probe_motion_step(voice,pat,1);
    wheel_probe_motion_tick(64);
    wheel_probe_motion_step(voice,pat,0);
    physical(550,615);audio(1);bend_is(voice,pb,.5f,.5f,0.0f);
    wheel_probe_motion_tick(32);audio(1);
    CHECK(engine_motion_value(pbl)==63 && engine_motion_value(pedal)==63);
    bend_is(voice,pb,.5f,.5f,0.0f);macro_is(voice,pb+1,1.0f);macro_is(voice,1,50.0f/127.0f);
    midi_bend(2,(unsigned)(2+voice),0);audio(1);
    wheel_probe_motion_tick(1);audio(1);
    macro_is(voice,pb,0.0f);macro_is(voice,pb+1,1.0f);
    physical(630,615);audio(1);macro_is(voice,pb,1.0f);
    wheel_probe_motion_release();audio(1);
    CHECK(engine_motion_value(pbl)==-1 && engine_motion_value(pedal)==-1);
    macro_is(voice,pb,1.0f);macro_is(voice,pb+1,1.0f);macro_is(voice,1,41.0f/127.0f);
    wheel_probe_motion_step(voice,pat,0);
    wheel_probe_motion_step(voice,(pat+1)%NPAT,0);audio(1);
    macro_is(voice,pb,1.0f);macro_is(voice,pb+1,1.0f);macro_is(voice,1,41.0f/127.0f);

    /* Individual performance-lane removal cannot enqueue a neutral restore.
     * It leaves a concurrently applied ordinary lane and the other part alone. */
    wheel_probe_motion_step(voice,pat,0);audio(1);
    CHECK(wheel_probe_queue()==0);
    engine_motion_clear(pbl);engine_motion_clear(pedal);
    CHECK(wheel_probe_queue()==0);
    CHECK(!proj.arr.lane[pbl].used && !proj.arr.lane[pedal].used && proj.arr.lane[ordinary].used);
    audio(1);macro_is(voice,pb,1.0f);macro_is(voice,pb+1,1.0f);macro_is(voice,1,100.0f/127.0f);
    CHECK(!memcmp(&other_saved,&proj.arr.lane[otherlane],sizeof other_saved));

    /* Recreate old lanes, then invoke the actual UI clear-lanes function. It
     * explicitly clears this part's ordinary lane as well, and restores only
     * that ordinary value. The other part and the patch stay byte-identical. */
    pbl=motion_alloc(proj.arr.lane,pat,part,T_303,voice,(int)pb);
    pedal=motion_alloc(proj.arr.lane,pat,part,T_303,voice,(int)pb+1);
    CHECK(pbl>=0 && pedal>=0);
    proj.arr.lane[pbl].val[0]=0;proj.arr.lane[pedal].val[0]=0;
    wheel_probe_motion_step(voice,pat,0);audio(1);
    clear_lanes(pat,part);audio(1);
    CHECK(!proj.arr.lane[pbl].used && !proj.arr.lane[pedal].used && !proj.arr.lane[ordinary].used);
    CHECK(motion_count(proj.arr.lane)==1 && !memcmp(&other_saved,&proj.arr.lane[otherlane],sizeof other_saved));
    macro_is(voice,pb,1.0f);macro_is(voice,pb+1,1.0f);macro_is(voice,1,41.0f/127.0f);
    CHECK(!memcmp(&saved,&proj.sound,sizeof saved));
    physical(550,615);audio(1);bend_is(voice,pb,.5f,.5f,0.0f);
    midi(0,(unsigned)(2+voice),0xb0,64,0);audio(1);macro_is(voice,pb+1,0.0f);
    select_kind(voice,(unsigned)(1-kind));
    CHECK(proj.arr.lane[otherlane].used && !memcmp(&other_saved,&proj.arr.lane[otherlane],sizeof other_saved));

    printf("PASS voice=%d kind=%s checks=%u physical-CQ-runtime=real adoption-preemptions=%u PCM=%016llx\n",
        voice,kind?"Miami":"Chicago",checks,preempted,(unsigned long long)pcm_hash);
    puts("PASS performance-only patch/motion; exact physical/MIDI center; held controls survive actual UI lane clear; ordinary patch/motion and other part retained");
    return 0;
}
