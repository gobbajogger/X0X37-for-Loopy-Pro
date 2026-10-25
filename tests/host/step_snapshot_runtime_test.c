/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual engine, sequencer sink and DSP; no preview callback substitutes. */
#include "../../firmware/src/app/engine.c"
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#include "../../experiments/wobble/wobble.h"

static int32_t pcm[512];
static unsigned checks;
#define CHECK(x) do { assert(x); ++checks; } while (0)
static void render_block(void) { engine_render(pcm, 256); }
static void effective(int b, uint8_t out[16], int identity)
{
    int id = 0;
    CHECK(engine_step_snapshot_values(b, out, &id));
    CHECK(id == identity);
}
static void expect_values(int b, const uint8_t expected[16], int identity)
{
    uint8_t out[16]; effective(b, out, identity);
    CHECK(!memcmp(out, expected, 16));
}
static void set_bank(int b, int id, uint8_t values[16], int offset)
{
    for (int i=0; i<16; ++i) {
        int max = id == 1 ? engine_param(T_303, b, i)->max : 127;
        values[i] = (uint8_t)((i*11 + offset) % (max+1));
        CHECK(engine_synth_param(b,id,i,values[i]));
    }
    render_block();
}
/* Drive the real sink at a chosen boundary, with the sequencer's current key.
 * Lifecycle remains the existing sequencer: no snapshot-specific timer. */
static void onset(int b, int step, int slide)
{
    int part = PART_303A+b;
    proj.pat[0].bass[b].step[step].flags = G_NOTE;
    seq.ppat[part] = 0;
    seq.t[part].pos = (uint8_t)step;
    s_step(0,part,step);
    s_bon(0,b,48+step,0,slide,100);
    seq.t[part].sounding = 1;
}
static void reset_engine(void)
{
    project_defaults();
    engine_init(proj.pat,&proj.arr.song,proj.arr.lane,&proj.sound);
    engine_apply_sound(&proj.sound);
    render_block();
}
static void capture_and_freeze(int b, int id)
{
    uint8_t saved[16], global[16], after[16];
    reset_engine();
    set_bank(b,id,saved,25);
    effective(b,after,id);
    CHECK(!memcmp(saved,after,16));
    CHECK(engine_step_snapshot_capture(b,0,0));
    const uint8_t *stored = motion_snapshot_read(proj.arr.lane,0,PART_303A+b,0);
    CHECK(stored && !memcmp(stored,saved,16));
    CHECK(motion_snapshot_engine(proj.arr.lane,0,PART_303A+b,0) == id);
    set_bank(b,id,global,59);
    onset(b,0,0);
    expect_values(b,saved,id);
    /* Every later global control changes its baseline, never the owned note. */
    set_bank(b,id,global,73);
    expect_values(b,saved,id);
    CHECK(!memcmp(motion_snapshot_read(proj.arr.lane,0,PART_303A+b,0),saved,16));
    /* Wrong-engine requests cannot reinterpret control indices. */
    CHECK(!engine_synth_param(b,id == 1 ? 2 : 1,0,10));
    expect_values(b,saved,id);
    /* Note off has a real audible tail: the gate is not an idle test. */
    s_boff(0,b);
    CHECK(!synth_active(&bass_slot[b]));
    CHECK(!synth_idle(&bass_slot[b]));
    set_bank(b,id,global,94);
    expect_values(b,saved,id);
    /* REST, mute and skipped attack preserve release-tail ownership. */
    int part = PART_303A+b;
    proj.pat[0].bass[b].step[1].flags=G_REST;
    s_step(0,part,1);
    expect_values(b,saved,id);
    seq.mute |= 1u << (MUTE_BASS0+b);
    proj.pat[0].bass[b].step[1].flags=G_TIE;
    s_step(0,part,1);
    expect_values(b,saved,id);
    seq.mute=0;
    prob[part][0]=0;
    onset(b,1,0);
    expect_values(b,saved,id);
    /* The sequencer remembers a logical sounding step even when probability
     * skipped its attack. A following TIE cannot steal the old audible tail. */
    CHECK(motion_snapshot_capture(proj.arr.lane,0,part,4,id,global)>=0);
    proj.pat[0].bass[b].step[4].flags=G_TIE;
    seq.t[part].sounding=1;
    CHECK(!synth_active(&bass_slot[b]));
    s_step(0,part,4);
    expect_values(b,saved,id);
    prob[part][0]=100;
    /* The next unlocked accepted attack restores the latest baseline. */
    onset(b,1,0);
    expect_values(b,global,id);
    CHECK(!snap_locked[b]);
    /* A pending parameter command cannot produce a misleading capture. */
    CHECK(engine_synth_param(b,id,0,7));
    lane_t before[NLANE]; memcpy(before,proj.arr.lane,sizeof before);
    CHECK(!engine_step_snapshot_capture(b,0,3));
    CHECK(!memcmp(before,proj.arr.lane,sizeof before));
    render_block();
    CHECK(engine_step_snapshot_capture(b,0,3));
    effective(b,after,id);
    CHECK(!memcmp(after,motion_snapshot_read(proj.arr.lane,0,part,3),16));
}
static void boundary_continuity(int b,int id)
{
    uint8_t a[16], other[16], expected[16];
    reset_engine();
    set_bank(b,id,a,38);
    CHECK(engine_step_snapshot_capture(b,0,0));
    memcpy(other,a,16);
    /* Isolate timbre from attack/pitch: only cutoff changes on tied boundary. */
    int cutoff = id == 1 ? BASS303_CUTOFF : W_CUTOFF;
    other[cutoff]=(uint8_t)((a[cutoff]+19)%128);
    CHECK(motion_snapshot_capture(proj.arr.lane,0,PART_303A+b,1,id,other)>=0);
    onset(b,0,0);
    render_block();
    int part=PART_303A+b;
    proj.pat[0].bass[b].step[1].flags=G_TIE;
    seq.t[part].sounding=1;
#if X0X_DEV_WOBBLE
    wobble_t old_wobble;
    if (id==2) old_wobble=dev_wobble;
#endif
    bass303_t old303=b303[b];
    s_step(0,part,1);
    expect_values(b,other,id);
    CHECK(synth_active(&bass_slot[b]));
    if (id==1) {
        CHECK(b303[b].phase==old303.phase);
        CHECK(b303[b].amp_y==old303.amp_y);
        CHECK(b303[b].env_y==old303.env_y);
        CHECK(b303[b].trig==old303.trig);
        CHECK(b303[b].amp_trig==old303.amp_trig);
    }
#if X0X_DEV_WOBBLE
    else {
        CHECK(dev_wobble.carrier_phase==old_wobble.carrier_phase);
        CHECK(dev_wobble.mod_phase==old_wobble.mod_phase);
        CHECK(dev_wobble.envelope==old_wobble.envelope);
        CHECK(dev_wobble.active_samples==old_wobble.active_samples);
    }
#endif
    /* A per-knob lock overlays native303 at the boundary, then stays frozen
     * while its motion ramp runs. Removal does not delete that lane. */
    int lane=motion_alloc(proj.arr.lane,0,part,T_303,b,cutoff);
    CHECK(lane>=0);
    proj.arr.lane[lane].val[1]=33;
    proj.arr.lane[lane].val[2]=77;
    s_step(0,part,1);
    memcpy(expected,other,16);
    if (id==1) expected[cutoff]=33;
    expect_values(b,expected,id);
    motion_tick(&mot,4096);
    expect_values(b,expected,id);
    CHECK(motion_snapshot_edit(proj.arr.lane,0,part,1,cutoff,56)>=0);
    CHECK(motion_snapshot_read(proj.arr.lane,0,part,1)[cutoff]==56);
    /* Stored edits become audible at a deliberate note/tie boundary only. */
    expect_values(b,expected,id);
    s_step(0,part,1);
    if (id==2) expected[cutoff]=56;
    expect_values(b,expected,id);
    motion_snapshot_remove(proj.arr.lane,0,part,1);
    CHECK(proj.arr.lane[lane].used && proj.arr.lane[lane].val[1]==33);
    /* Removing storage cannot mutate the ISR-owned current note copy. */
    expect_values(b,expected,id);
    /* Same-engine slide is dispatched without panicking/reinitializing voice. */
    onset(b,0,1);
    CHECK(synth_active(&bass_slot[b]));
    CHECK(snap_locked[b]);
}
/* Differential PCM oracle: apply the same value to a copied existing voice,
 * with no lifecycle callback. Any hidden reset/retrigger/drop on the snapshot
 * path then changes the actual PCM. This covers separate discontinuity controls;
 * equality is source continuity evidence, not a target timing/listening claim. */
static void pcm_boundaries(int b,int id)
{
    reset_engine();
    uint8_t values[16];
    effective(b,values,id);
    const int native_controls[]={BASS303_CUTOFF,BASS303_RESO,BASS303_VOLUME,BASS303_WAVE};
    const int miami_controls[]={W_CUTOFF,W_CARRIER_LEVEL,W_OUTPUT,W_CARRIER_WAVE};
    const int *controls=id==1?native_controls:miami_controls;
    CHECK(motion_snapshot_capture(proj.arr.lane,0,PART_303A+b,0,id,values)>=0);
    onset(b,0,0); render_block();
    float actual[256],expected[256];
    for (int c=0;c<4;c++) {
        int param=controls[c];
        int max=id==1?engine_param(T_303,b,param)->max:127;
        values[param]=(uint8_t)((values[param]+max/2+1)%(max+1));
        CHECK(motion_snapshot_capture(proj.arr.lane,0,PART_303A+b,1,id,values)>=0);
        bass303_t reference303=b303[b];
        synth_slot_t reference=bass_slot[b];
        if (id==1) reference.active.context=&reference303;
#if X0X_DEV_WOBBLE
        wobble_t reference_wobble;
        if (id==2) { reference_wobble=dev_wobble; reference.active.context=&reference_wobble; }
#endif
        reference.active.ops->set(reference.active.context,param,values[param]);
        proj.pat[0].bass[b].step[1].flags=G_TIE;
        seq.t[PART_303A+b].sounding=1;
        s_step(0,PART_303A+b,1);
        synth_render(&bass_slot[b],actual,256);
        synth_render(&reference,expected,256);
        CHECK(!memcmp(actual,expected,sizeof actual));
        /* Flood global changes while the note is locked. Drain may consume them,
         * but neither the current note nor its raw generated PCM may change. */
        reference303=b303[b];
        if (id==1) reference.active.context=&reference303;
#if X0X_DEV_WOBBLE
        if (id==2) { reference_wobble=dev_wobble; reference.active.context=&reference_wobble; }
#endif
        for(int i=0;i<16;i++) CHECK(engine_synth_param(b,id,i,0));
        drain();
        synth_render(&bass_slot[b],actual,256);
        synth_render(&reference,expected,256);
        CHECK(!memcmp(actual,expected,sizeof actual));
    }
}
static void fixed_capacity(int b,int id)
{
    reset_engine();
    uint8_t values[16]; effective(b,values,id);
    for (int k=0;k<NLANE;k++) {
        CHECK(motion_snapshot_capture(proj.arr.lane,k/NSTEPS,PART_303A+b,k%NSTEPS,id,values)>=0);
    }
    lane_t before[NLANE]; memcpy(before,proj.arr.lane,sizeof before);
    CHECK(!engine_step_snapshot_capture(b,3,0));
    CHECK(!memcmp(before,proj.arr.lane,sizeof before));
    CHECK(motion_snapshot_edit(proj.arr.lane,0,PART_303A+b,0,0,12)>=0);
    CHECK(motion_count(proj.arr.lane)==NLANE);
    CHECK(motion_snapshot_read(proj.arr.lane,0,PART_303A+b,0)[0]==12);
}
static void midi_attack(int b)
{
    seq.ch[PART_303A+b]=8;
    uint32_t pkt=9u | ((uint32_t)(0x90|8)<<8) | (60u<<16) | (100u<<24);
    midi_in(pkt);
}
static void live_attack_paths(int b,int id)
{
    reset_engine();
    uint8_t base[16],current[16],locked[16];
    set_bank(b,id,base,45);
    int param=id==1?BASS303_CUTOFF:W_CUTOFF;
    memcpy(current,base,16);
    if(id==1) { mot_apply(0,T_303,b,param,44); current[param]=44; }
    engine_bass_on_velocity(b,55,0,0,100); drain();
    expect_values(b,current,id);
    midi_attack(b);
    expect_values(b,current,id);
    int k=-1;
    if(id==1) {
        k=motion_alloc(proj.arr.lane,0,PART_303A+b,T_303,b,param);
        CHECK(k>=0);
        proj.arr.lane[k].val[1]=33;
    }
    memcpy(locked,base,16); locked[param]=88;
    CHECK(motion_snapshot_capture(proj.arr.lane,0,PART_303A+b,0,id,locked)>=0);
    for(int route=0;route<2;route++) {
        onset(b,0,0);
        expect_values(b,locked,id);
        /* A motion value advances behind a frozen snapshot and must be resumed
         * by either live input route when that new note takes ownership. */
        if(id==1) motion_step(&mot,PART_303A+b,0,1,2,16);
        expect_values(b,locked,id);
        if(route==0) { engine_bass_on_velocity(b,59,0,1,100); drain(); }
        else midi_attack(b);
        memcpy(current,base,16);
        if(id==1) current[param]=33;
        expect_values(b,current,id);
        CHECK(!snap_locked[b]);
        CHECK(synth_active(&bass_slot[b]));
    }
}
static void native_validation_and_unlocked_motion(void)
{
    reset_engine();
    int b=1;
    uint8_t values[16];
    set_bank(b,1,values,51);
    CHECK(!memcmp(b303[b].pot,values,16));
    uint32_t before_w=cq_w;
    CHECK(!engine_synth_param(b,1,BASS303_WAVE,2));
    CHECK(!engine_synth_param(b,1,BASS303_DRVTYPE,3));
    CHECK(cq_w==before_w);
    CHECK(!memcmp(b303[b].pot,values,16));
    /* Legacy command clamp must reflect actual DSP pot bytes in the capture. */
    engine_set(T_303,b,BASS303_WAVE,127);
    render_block();
    values[BASS303_WAVE]=engine_param(T_303,b,BASS303_WAVE)->max;
    expect_values(b,values,1);
    CHECK(!memcmp(b303[b].pot,values,16));
    /* Existing unlocked notes must retain a currently applied motion value,
     * including a main-loop hold gesture, instead of reapplying the base. */
    mot_apply(0,T_303,b,BASS303_CUTOFF,44);
    onset(b,0,0);
    values[BASS303_CUTOFF]=44;
    expect_values(b,values,1);
    int k=motion_alloc(proj.arr.lane,0,PART_303A+b,T_303,b,BASS303_CUTOFF);
    CHECK(k>=0);
    proj.arr.lane[k].val[1]=17;
    engine_set(T_303,b,BASS303_CUTOFF,63);
    render_block();
    motion_req_hold(&mot,k);
    onset(b,1,0);
    values[BASS303_CUTOFF]=63;
    expect_values(b,values,1);
    CHECK(b303[b].pot[BASS303_CUTOFF]==63);
}
static void wrong_identity_and_tail_idle(int b,int id)
{
    reset_engine();
    uint8_t saved[16],global[16];
    effective(b,global,id);
    memset(saved,13,16);
    CHECK(motion_snapshot_capture(proj.arr.lane,0,PART_303A+b,0,id==1?2:1,saved)>=0);
    onset(b,0,0);
    expect_values(b,global,id);
    CHECK(!snap_locked[b]);
    CHECK(motion_snapshot_capture(proj.arr.lane,0,PART_303A+b,0,id,global)>=0);
    onset(b,0,0); render_block(); s_boff(0,b);
    CHECK(!synth_idle(&bass_slot[b]));
    CHECK(engine_synth_param(b,id,0,19)); render_block();
    expect_values(b,global,id);
    unsigned blocks=0;
    while (!synth_idle(&bass_slot[b]) && blocks++<20000) render_block();
    CHECK(synth_idle(&bass_slot[b]));
    render_block();
    global[0]=19;
    expect_values(b,global,id);
    CHECK(!snap_locked[b]);
}
int main(void)
{
    int id0=1;
#if X0X_DEV_WOBBLE
    id0=2;
#endif
    capture_and_freeze(0,id0);
    capture_and_freeze(1,1);
    boundary_continuity(0,id0);
    boundary_continuity(1,1);
    pcm_boundaries(0,id0);
    pcm_boundaries(1,1);
    fixed_capacity(0,id0);
    live_attack_paths(0,id0);
    live_attack_paths(1,1);
    native_validation_and_unlocked_motion();
    wrong_identity_and_tail_idle(0,id0);
    wrong_identity_and_tail_idle(1,1);
    printf("PASS actual step snapshot engine: %u assertions, backend0=%s, native303B, sixteen effective values, frozen global/tail, tie/slide state, control-specific differential PCM, motion precedence, identity, queue coherence, fixed-capacity rejection, idle restoration\n",checks,id0==2?"Miami":"303");
    return 0;
}
