/* SPDX-License-Identifier: GPL-3.0-only */
/* Whole production UI/project/engine, with the existing simulated platform.
 * Archived Drone pattern bytes are storage, not a playable engine selection.
 * No physical flash, interrupt timing or device behavior is exercised here. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/original_runtime.h"
#include "../../firmware/src/seq/held_chord.h"

static unsigned checks, cases;
static const char *phase = "boot";
static int tested_voice, tested_factory;
static project_t saved_project;
static int32_t pcm[512];
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr,"FAIL %s A/B=%d factory=%d line %d: %s\n", \
        phase,tested_voice,tested_factory,__LINE__,#x); exit(1); } } while (0)

static uint64_t audio(unsigned blocks)
{
    uint64_t h = UINT64_C(1469598103934665603);
    for (unsigned b=0;b<blocks;++b) {
        engine_render(pcm,256);
        for (unsigned i=0;i<512;++i) h=(h^(uint32_t)pcm[i])*UINT64_C(1099511628211);
    }
    return h;
}
static void frames(unsigned total)
{
    while(total) {
        unsigned n=total>256?256:total;
        engine_render(pcm,n);total-=n;
    }
}
static void settle(void) { (void)audio(12); }
static void midi(unsigned voice,unsigned source,unsigned note,unsigned velocity)
{
    unsigned st=(velocity?0x90u:0x80u)|seq.ch[TRK_BASS0+voice];
    min_q[mi_w++%MQ]=(st>>4)|(source<<4)|(st<<8)|(note<<16)|(velocity<<24);
}
static void fresh(int voice)
{
    tested_voice=voice;
    /* Independent cases start at an explicit panic/cold boundary. Initialized
     * adoption now correctly preserves debt and cannot serve as a test reset. */
    engine_request_notes_off();frames(256);
    engine_project_hold(1); engine_project_flush();
    project_defaults();
    for(int v=0;v<NBASS;++v) {
        CHECK(original_sound_init(proj.sound.v[T_303][v],ORIGINAL_SOUND_CHICAGO,0));
        engine_bass_sound_refresh(&proj.sound,v);
    }
    engine_init(proj.pat,&proj.arr.song,proj.arr.lane,&proj.sound);
    engine_apply_sound(&proj.sound);
    project_apply_io();
    engine_project_release();
    ui_init();
    engine_smc_solo((uint8_t)(1u<<(PART_303A+voice)));
    for(int p=0;p<NPAT;++p) for(int v=0;v<NBASS;++v) {
        proj.pat[p].rsv[v]=0;
        for(int s=0;s<NSTEPS;++s)proj.pat[p].bass[v].step[s].flags=G_REST;
    }
    engine_note_lock_capture(0);
    engine_bass_wheel_owners(0);
    settle();
    pattern_sound_tick();
    CHECK(engine_bass_status(voice)==ORIGINAL_RUNTIME_OK);
    CHECK(engine_bass_bank(voice)==ORIGINAL_SOUND_CHICAGO);
    CHECK(!engine_bass_preset_pending(voice));
}
static void unavailable_now(int voice,int factory)
{
    int t=TRK_BASS0+voice;
    proj.pat[1].rsv[voice]=(uint8_t)(0xC1u+(unsigned)factory);
    seq.ppat[t]=1;seq.pcue[t]=0xFF;
    pattern_sound_tick();
    (void)audio(1);
    CHECK(engine_bass_status(voice)==ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(engine_bass_bank(voice)==ORIGINAL_SOUND_DRONE);
    CHECK(engine_bass_preset_index(voice)==factory);
    CHECK(!engine_bass_preset_pending(voice));
}
/* This same case compiles against the previous production pattern policy.
 * DRONE_PATTERN_CONTROL_ONLY excludes new API calls elsewhere in this file;
 * expected RED must be a runtime assertion, never a missing API/link failure. */
static void terminal_regression(int voice)
{
    ++cases;phase="actual Drone pattern stops the old audible engine";
    fresh(voice);
    midi((unsigned)voice,0,60,110);(void)audio(4);
    CHECK(engine_original_keys(voice,0)==1);
    int heard=0;for(unsigned i=0;i<512;++i)heard|=pcm[i]!=0;
    CHECK(heard);
    proj.pat[1].rsv[voice]=0xC1u;
    seq.ppat[TRK_BASS0+voice]=1;seq.pcue[TRK_BASS0+voice]=0xFF;
    pattern_sound_tick();(void)audio(1);
    if(engine_bass_status(voice)!=ORIGINAL_RUNTIME_UNAVAILABLE) {
        unsigned nonzero=0;for(unsigned i=0;i<512;++i)nonzero+=pcm[i]!=0;
        fprintf(stderr,"terminal observation: status=%d bank=%d pending=%d held=%d nonzero_samples=%u/512\n",
            engine_bass_status(voice),engine_bass_bank(voice),engine_bass_preset_pending(voice),
            engine_bass_keys_held(),nonzero);
    }
    CHECK(engine_bass_status(voice)==ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(engine_bass_bank(voice)==2 && engine_bass_preset_index(voice)==0);
    CHECK(!engine_bass_preset_pending(voice));
    CHECK(engine_original_keys(voice,0)==0);
    (void)audio(64);
    for(unsigned i=0;i<512;++i)CHECK(pcm[i]==0);
}

#ifndef DRONE_PATTERN_CONTROL_ONLY
static uint32_t bits(float f) { uint32_t n;memcpy(&n,&f,4);return n; }
static void wait_applied(int v)
{
    for(unsigned i=0;i<4096 && engine_bass_preset_pending(v);++i)(void)audio(1);
    CHECK(!engine_bass_preset_pending(v));
}
static void state(int v,uint32_t *g,uint32_t *c)
{ int arp;engine_live_key_state(v,g,&arp,c);CHECK(arp==0); }
static void base_matches(int v,const uint8_t cell[32])
{
    for(unsigned p=0;p<original_sound_count(cell[29]);++p) {
        float value;
        if(original_sound_is_performance(cell[29],p))continue;
        CHECK(original_sound_value(cell,p,&value));
        CHECK(engine_original_value_bits(v,p)==bits(value));
    }
}
static void preserve(void) { saved_project=proj; }
static void preserved(void)
{
    CHECK(!memcmp(&saved_project.sound,&proj.sound,sizeof proj.sound));
    CHECK(!memcmp(saved_project.pat,proj.pat,sizeof proj.pat));
    CHECK(!memcmp(&saved_project.arr,&proj.arr,sizeof proj.arr));
}
static void edited_base(int v)
{
    CHECK(original_sound_set(proj.sound.v[T_303][v],0,101));
    CHECK(original_sound_set(proj.sound.v[T_303][v],3,17));
    engine_bass_sound_refresh(&proj.sound,v);
    proj.sound.v[T_MIX][PART_303A+v][MX_LEVEL]=97;
    proj.sound.v[T_MIX][PART_303A+v][MX_REV]=0;
    proj.sound.v[T_MIX][PART_303A+v][MX_DLY]=0;
    for(int p=0;p<MX_NPARAMS;++p)
        engine_set(T_MIX,PART_303A+v,p,proj.sound.v[T_MIX][PART_303A+v][p]);
    engine_set_base(T_303,v,0);engine_set_base(T_303,v,3);settle();
    CHECK(engine_bass_preset_edited(v));
    base_matches(v,proj.sound.v[T_303][v]);
}
static void archived_lanes(int v,int factory)
{
    /* A normal motion lane and held chord use the real project lane pool. */
    int k=motion_alloc(proj.arr.lane,1,PART_303A+v,T_303,v,0);
    CHECK(k>=0);proj.arr.lane[k].val[0]=37;proj.arr.lane[k].val[3]=81;
    uint8_t chord[]={48,55,60};
    CHECK(held_chord_set(proj.arr.lane,1,v,4,chord,3));
    proj.pat[1].bass[v].step[4]=(bstep_t){48,G_NOTE,109};
    for(k=0;k<NLANE && proj.arr.lane[k].used;++k){}
    CHECK(k<NLANE);
    lane_t *l=&proj.arr.lane[k];memset(l,0,sizeof *l);
    l->used=1;l->gen=73;l->pat=1;l->part=(uint8_t)(PART_303A+v);
    l->t=BASS_NOTE_TARGET;l->v=4;l->i=BASS_NOTE_VERSION;
    CHECK(original_sound_init(l->val,ORIGINAL_SOUND_DRONE,(unsigned)factory));
    CHECK(original_sound_set(l->val,0,119));
    static const bass_identity_entry_t registry[]={
        {ORIGINAL_SOUND_CHICAGO_ID,ORIGINAL_SOUND_SCHEMA,22,1},
        {ORIGINAL_SOUND_MIAMI_ID,ORIGINAL_SOUND_SCHEMA,18,1},
        {ORIGINAL_SOUND_DRONE_ID,ORIGINAL_SOUND_SCHEMA,8,0},
        {ORIGINAL_SOUND_JUNO_ID,ORIGINAL_SOUND_SCHEMA,18,1}
    };
    bass_sound_info_t tag={{ORIGINAL_SOUND_DRONE_ID,ORIGINAL_SOUND_SCHEMA,8,BASS_IDENTITY_KNOWN},
        (uint16_t)factory,BASS_SOUND_EDITED};
    CHECK(bass_sound_tag_encode((bass_sound_tag_t *)(l->val+BASS_NOTE_TAG),&tag,registry,4)==BASS_IDENTITY_UNAVAILABLE);
    for(int p=0;p<MX_NPARAMS;++p)l->val[BASS_NOTE_MIX+p]=proj.sound.v[T_MIX][PART_303A+v][p];
    l->val[BASS_NOTE_MIX+MX_LEVEL]=0; /* must never become the base on a refused onset */
    CHECK(engine_bass_note_validate(l)==0);
    engine_note_lock_refresh();
}
static void assert_silent(void)
{
    (void)audio(64);
    for(unsigned i=0;i<512;++i)CHECK(pcm[i]==0);
}
static void wait_step(int v,int step)
{
    for(unsigned n=0;n<2000;++n) {
        (void)audio(1);
        if(seq.playing && seq.t[TRK_BASS0+v].started && eng_step[TRK_BASS0+v]==step)return;
    }
    CHECK(0);
}
static void unavailable_archive(int v,int factory)
{
    ++cases;phase="terminal archive preserves base/tag/motion/chord/note-lock and round-trips";
    fresh(v);edited_base(v);archived_lanes(v,factory);
    uint32_t base_level=engine_mix_level_bits(PART_303A+v);
    int t=TRK_BASS0+v;
    proj.pat[1].rsv[v]=(uint8_t)(0xC1u+(unsigned)factory);
    preserve();
    CHECK(project_save()==0);
    seq_start(&seq);(void)audio(1); /* queued cue must await a running pattern boundary */
    uint32_t g,c,g1,c1;state(v,&g,&c);
    uint32_t serial=engine_bass_choice_serial();
    midi((unsigned)v,0,60,110);(void)audio(4);
    CHECK(engine_original_keys(v,0)==1);
    /* Terminal unavailability bypasses old ownership, wheel and motion locks. */
    engine_bass_wheel_owners((uint8_t)(1u<<v));
    seq.pcue[t]=1;pattern_sound_tick();
    CHECK(engine_bass_choice_serial()==serial+1);
    CHECK(engine_bass_requested_bank(v)==ORIGINAL_SOUND_DRONE);
    CHECK(engine_bass_requested_preset(v)==factory);
    CHECK(engine_bass_preset_gate(v)==1);
    CHECK(engine_bass_preset_pending(v)==1);
    (void)audio(1);
    CHECK(engine_bass_status(v)==ORIGINAL_RUNTIME_OK && engine_bass_bank(v)==ORIGINAL_SOUND_CHICAGO);
    CHECK(engine_original_keys(v,0)==1);
    state(v,&g1,&c1);CHECK(g1==g && c1==c);
    seq.ppat[t]=1;seq.pcue[t]=0xFF;(void)audio(1);
    CHECK(engine_bass_status(v)==ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(engine_bass_bank(v)==2 && engine_bass_preset_index(v)==factory);
    CHECK(!engine_bass_preset_pending(v));
    CHECK(!engine_original_keys(v,0) && !(engine_midi_held()&(1u<<v)));
    state(v,&g1,&c1);CHECK(g1==g+1 && c1==c+1);
    uint32_t applied_serial=engine_bass_choice_serial();
    for(unsigned i=0;i<4;++i){pattern_sound_tick();(void)audio(1);}
    CHECK(engine_bass_choice_serial()==applied_serial);
    state(v,&g,&c);CHECK(g==g1 && c==c1);
    float level=9,mod=9;unsigned envelope=99,down=99;
    CHECK(!engine_bass_voice_view(v,&level,&mod,&envelope,&down));
    CHECK(level==0 && mod==0 && down==0);
    seq_stop(&seq);(void)audio(1);
    preserved();assert_silent();
    engine_bass_wheel_owners(0);
    /* Live admission must not modify archived project bytes or the request epoch. */
    serial=engine_bass_choice_serial();uint8_t dirty=ui.dirty;
    CHECK(engine_bass_select_preset(v,ORIGINAL_SOUND_DRONE,factory,&proj.sound)==-2);
    CHECK(engine_bass_choice_serial()==serial && ui.dirty==dirty);preserved();
    engine_note_lock_capture(1);
    CHECK(engine_bass_note_capture(proj.arr.lane,1,v,7,&proj.sound)<0);
    uint8_t newchord[]={62,65,69};
    CHECK(!seq_bass_chord_set(&seq,v,1,7,newchord,-3));
    CHECK(!seq_bass_chord_set(&seq,v,1,7,newchord,3));
    preserved();
    /* Actual sequenced Drone lock onsets cannot change the saved mixer. */
    seq.ppat[t]=1;seq_start(&seq);wait_step(v,4);
    CHECK(engine_mix_level_bits(PART_303A+v)==base_level); /* before Stop can restore anything */
    seq_stop(&seq);(void)audio(2);
    CHECK(engine_bass_status(v)==ORIGINAL_RUNTIME_UNAVAILABLE);preserved();
    CHECK(project_save()==0);
    engine_project_hold(1);CHECK(project_load_stage()==0);
    CHECK(!memcmp(&saved_project.sound,&project_stage.sound,sizeof project_stage.sound));
    CHECK(!memcmp(saved_project.pat,project_stage.pat,sizeof project_stage.pat));
    CHECK(!memcmp(&saved_project.arr,&project_stage.arr,sizeof project_stage.arr));
    CHECK(project_validate_stage()==0);engine_project_release();
    /* OFF is a future gated inheritance request, never an early unmute. */
    seq_start(&seq);(void)audio(1);
    seq.pcue[t]=2;pattern_sound_tick();CHECK(engine_bass_preset_gate(v)==2);
    CHECK(engine_bass_preset_pending(v)==1);(void)audio(2);
    CHECK(engine_bass_status(v)==ORIGINAL_RUNTIME_UNAVAILABLE);
    seq.pcue[t]=0xFF;pattern_sound_tick();CHECK(!engine_bass_preset_pending(v));
    assert_silent();CHECK(engine_bass_status(v)==ORIGINAL_RUNTIME_UNAVAILABLE);
    seq.pcue[t]=2;pattern_sound_tick();seq.ppat[t]=2;seq.pcue[t]=0xFF;(void)audio(1);
    CHECK(engine_bass_status(v)==ORIGINAL_RUNTIME_OK && engine_bass_bank(v)==0);
    CHECK(engine_bass_preset_index(v)==saved_project.sound.v[T_303][v][30]);
    CHECK(!engine_bass_preset_pending(v));base_matches(v,saved_project.sound.v[T_303][v]);preserved();
    /* Explicit Stop above uses the established all-off lifecycle. A new note
     * still sounds with the inherited base and its own off ends it normally. */
    midi((unsigned)v,0,60,110);(void)audio(4);CHECK(engine_original_keys(v,0)==1);
    int heard=0;for(unsigned i=0;i<512;++i)heard|=pcm[i]!=0;CHECK(heard);
    midi((unsigned)v,0,60,0);(void)audio(1);CHECK(engine_original_keys(v,0)==0);
    engine_note_lock_capture(0);
}

static uint64_t future_audio(int v,int cue_kind)
{
    /* Warm engine_init preserves the Chicago accent capacitor. The supported
     * non-Chicago transition clears it before a second fresh init resets all
     * shared FX/master state. Both branches therefore start identically. */
    fresh(v);CHECK(engine_bass_select_preset(v,ORIGINAL_SOUND_MIAMI,0,&proj.sound)==0);
    wait_applied(v);(void)audio(2);
    fresh(v);seq_start(&seq);(void)audio(1);
    midi((unsigned)v,0,57,93);(void)audio(4);
    if(cue_kind) {
        proj.pat[1].rsv[v]=cue_kind==1?0xC2u:0;
        seq.pcue[TRK_BASS0+v]=1;pattern_sound_tick();
    }
    return audio(8);
}
static void cancellation_and_forgery(int v)
{
    ++cases;phase="future cue preserves actual PCM and withdrawal/edit never mutes";
    uint64_t carry=future_audio(v,0),future=future_audio(v,1);
    CHECK(carry==future);
    uint32_t g,c,g1,c1;state(v,&g,&c);
    seq.pcue[TRK_BASS0+v]=0xFF;pattern_sound_tick();
    CHECK(!engine_bass_preset_pending(v));(void)audio(2);
    CHECK(engine_bass_bank(v)==0 && engine_bass_status(v)==1 && engine_original_keys(v,0)==1);
    state(v,&g1,&c1);CHECK(g==g1 && c==c1);
    sound_t before=proj.sound;uint32_t serial=engine_bass_choice_serial();
    CHECK(engine_bass_recall_unavailable_at(v,2,0,&proj.sound,3)<0);
    CHECK(engine_bass_recall_unavailable_at(v,0,0,&proj.sound,3)<0);
    CHECK(engine_bass_recall_unavailable_at(v,2,4,&proj.sound,3)<0);
    CHECK(engine_bass_recall_unavailable_at(v,2,0,&before,3)<0);
    CHECK(engine_bass_choice_serial()==serial && !memcmp(&before,&proj.sound,sizeof before));
    for(unsigned raw=0;raw<256;++raw) {
        if(raw>=0xC1u && raw<=0xC4u)continue;
        proj.pat[3].rsv[v]=(uint8_t)raw;
        CHECK(engine_bass_recall_unavailable_at(v,2,0,&proj.sound,3)<0);
        CHECK(engine_bass_choice_serial()==serial);
    }
    for(unsigned other=1;other<4;++other) {
        proj.pat[3].rsv[v]=(uint8_t)(0xC1u+other);
        CHECK(engine_bass_recall_unavailable_at(v,2,0,&proj.sound,3)<0);
        CHECK(engine_bass_choice_serial()==serial);
    }
    proj.pat[3].rsv[v]=0;
    proj.pat[1].rsv[v]=0xC1u;seq.pcue[TRK_BASS0+v]=1;pattern_sound_tick();
    CHECK(engine_bass_preset_pending(v)==1);
    /* The UI already inspected this byte; edit it before the actual audio boundary. */
    proj.pat[1].rsv[v]=0;seq.ppat[TRK_BASS0+v]=1;seq.pcue[TRK_BASS0+v]=0xFF;
    (void)audio(1);CHECK(!engine_bass_preset_pending(v));
    CHECK(engine_bass_status(v)==1 && engine_bass_bank(v)==0 && engine_original_keys(v,0)==1);
    state(v,&g1,&c1);CHECK(g==g1 && c==c1);
}

static void available_supersedes(int v)
{
    ++cases;phase="available factory and explicit choices supersede unavailable state";
    fresh(v);unavailable_now(v,1);seq_start(&seq);(void)audio(1);
    proj.pat[2].rsv[v]=(uint8_t)bass_pattern_preset_encode(ORIGINAL_SOUND_JUNO,6);
    seq.pcue[TRK_BASS0+v]=2;pattern_sound_tick();(void)audio(2);
    CHECK(engine_bass_status(v)==-3 && engine_bass_preset_pending(v));
    seq.ppat[TRK_BASS0+v]=2;seq.pcue[TRK_BASS0+v]=0xFF;wait_applied(v);
    CHECK(engine_bass_bank(v)==3 && engine_bass_status(v)==1 && engine_bass_preset_index(v)==5);
    CHECK(engine_bass_sound_bank(&proj.sound,v)==3);
    fresh(v);unavailable_now(v,2);
    CHECK(engine_bass_select_preset(v,ORIGINAL_SOUND_MIAMI,4,&proj.sound)==0);wait_applied(v);
    CHECK(engine_bass_bank(v)==1 && engine_bass_status(v)==1 && engine_bass_preset_index(v)==4);
    CHECK(engine_bass_sound_bank(&proj.sound,v)==1);
    /* A newer available request replaces a queued silent cue before adoption. */
    fresh(v);proj.pat[1].rsv[v]=0xC4u;seq.pcue[TRK_BASS0+v]=1;pattern_sound_tick();
    CHECK(engine_bass_select_preset(v,0,5,&proj.sound)==0);wait_applied(v);
    CHECK(engine_bass_bank(v)==0 && engine_bass_status(v)==1 && engine_bass_preset_index(v)==5);
    seq.ppat[TRK_BASS0+v]=1;seq.pcue[TRK_BASS0+v]=0xFF;(void)audio(2);
    CHECK(engine_bass_bank(v)==0 && engine_bass_status(v)==1 && engine_bass_preset_index(v)==5);
}
static void project_replacement(int v)
{
    ++cases;phase="project replacement clears pending and transient unavailable recall";
    for(int applied=0;applied<2;++applied) {
        fresh(v);edited_base(v);CHECK(project_save()==0);
        proj.pat[1].rsv[v]=0xC3u;seq.pcue[TRK_BASS0+v]=1;pattern_sound_tick();
        if(applied){seq.ppat[TRK_BASS0+v]=1;seq.pcue[TRK_BASS0+v]=0xFF;(void)audio(1);CHECK(engine_bass_status(v)==-3);}
        else CHECK(engine_bass_preset_pending(v)==1);
        CHECK(ui_project_load()==0);settle();
        CHECK(!engine_bass_preset_pending(v) && engine_bass_status(v)==1 && engine_bass_bank(v)==0);
        base_matches(v,proj.sound.v[T_303][v]);
        uint32_t serial=engine_bass_choice_serial();pattern_sound_tick();(void)audio(2);
        CHECK(engine_bass_choice_serial()==serial && !engine_bass_preset_pending(v));
        CHECK(proj.pat[1].rsv[v]==0 && engine_bass_status(v)==1);
        seq.ppat[TRK_BASS0+v]=1;pattern_sound_tick();(void)audio(2);
        CHECK(engine_bass_status(v)==1 && engine_bass_bank(v)==0);
    }
}
static void archived_base_project(int v,int factory)
{
    ++cases;phase="saved Drone base stays byte-exact and unavailable through project load";
    fresh(v);
    CHECK(original_sound_init(proj.sound.v[T_303][v],2,(unsigned)factory));
    CHECK(original_sound_set(proj.sound.v[T_303][v],0,119));
    CHECK(original_sound_set(proj.sound.v[T_303][v],4,3));
    proj.sound.v[T_303][v][6]=113;proj.sound.v[T_303][v][7]=91;
    proj.sound.v[T_303][v][22]|=0xC0u;
    engine_bass_sound_refresh(&proj.sound,v);
    archived_lanes(v,factory);
    engine_apply_sound(&proj.sound);settle();
    CHECK(engine_bass_sound_validate(&proj.sound)==0);
    CHECK(engine_bass_status(v)==-3 && engine_bass_bank(v)==2);
    proj.pat[1].rsv[v]=(uint8_t)(0xC1u+(unsigned)((factory+1)%4));preserve();
    CHECK(project_save()==0 && ui_project_load()==0);settle();preserved();
    CHECK(engine_bass_status(v)==-3 && engine_bass_bank(v)==2);
    pattern_sound_tick();(void)audio(1);CHECK(!engine_bass_preset_pending(v));
    unavailable_now(v,(factory+1)%4); /* runtime selection may differ from the saved base */
    seq.ppat[TRK_BASS0+v]=2;pattern_sound_tick();(void)audio(1);
    CHECK(engine_bass_status(v)==-3 && engine_bass_bank(v)==2);
    CHECK(engine_bass_preset_index(v)==factory && !engine_bass_preset_pending(v));
    preserved();assert_silent();
}
static void normal_quiescence(int v)
{
    ++cases;phase="ordinary available factory recall retains existing quiescence";
    fresh(v);midi((unsigned)v,0,60,110);(void)audio(4);
    proj.pat[1].rsv[v]=(uint8_t)bass_pattern_preset_encode(0,6);
    seq.ppat[TRK_BASS0+v]=1;pattern_sound_tick();
    CHECK(engine_bass_preset_pending(v)==2);(void)audio(2);
    CHECK(engine_bass_preset_index(v)==0 && engine_original_keys(v,0)==1);
    engine_bass_wheel_owners((uint8_t)(1u<<v));CHECK(engine_bass_preset_pending(v)==3);
    midi((unsigned)v,0,60,0);(void)audio(1);engine_bass_wheel_owners(0);wait_applied(v);
    CHECK(engine_bass_preset_index(v)==5 && engine_bass_bank(v)==0 && engine_bass_status(v)==1);
    fresh(v);int k=motion_alloc(proj.arr.lane,0,PART_303A+v,T_303,v,0);CHECK(k>=0);
    proj.arr.lane[k].val[0]=81;
    CHECK(engine_bass_select_preset(v,0,5,&proj.sound)==-4);
}
static void held_key_and_lock(int v)
{
    ++cases;phase="accepted note lock and physical ownership retire at terminal boundary";
    fresh(v);
    engine_note_lock_capture(1);
    CHECK(engine_bass_note_capture(proj.arr.lane,0,v,0,&proj.sound)==0);
    int lock=bass_note_find(proj.arr.lane,0,v,0);CHECK(lock>=0);
    uint8_t locked[32];memcpy(locked,proj.arr.lane[lock].val,32);
    proj.arr.lane[lock].val[BASS_NOTE_MIX+MX_LEVEL]=0;
    edited_base(v);
    uint32_t base_level=engine_mix_level_bits(PART_303A+v);
    proj.pat[0].bass[v].step[0]=(bstep_t){48,G_NOTE,110};
    engine_note_lock_refresh();seq_start(&seq);(void)audio(1);
    base_matches(v,locked); /* the accepted old onset actually applied its lock */
    CHECK(engine_mix_level_bits(PART_303A+v)==bits(0.0f));
    uint32_t oldg,oldc,newg,newc;state(v,&oldg,&oldc);
    CHECK(engine_live_key(v,oldg,1,60,110,0,0));(void)audio(1);
    CHECK(engine_bass_keys_held()&(1u<<v));
    proj.pat[1].rsv[v]=0xC4;preserve();
    seq.ppat[TRK_BASS0+v]=1;seq.pcue[TRK_BASS0+v]=0xFF;pattern_sound_tick();(void)audio(1);
    CHECK(engine_bass_status(v)==-3 && !(engine_bass_keys_held()&(1u<<v)));
    CHECK(engine_mix_level_bits(PART_303A+v)==base_level);
    state(v,&newg,&newc);CHECK(newg==oldg+1 && newc==oldc+1);preserved();
    seq_stop(&seq);(void)audio(1);
    /* An inherited available base must not keep the old lock's zero mixer. */
    seq.ppat[TRK_BASS0+v]=2;pattern_sound_tick();(void)audio(1);
    CHECK(engine_bass_status(v)==1);base_matches(v,proj.sound.v[T_303][v]);
    state(v,&newg,&newc);
    CHECK(engine_live_key(v,newg,2,60,110,0,0));(void)audio(4);
    CHECK(engine_bass_keys_held()&(1u<<v));
    int heard=0;for(unsigned i=0;i<512;++i)heard|=pcm[i]!=0;CHECK(heard);
    CHECK(engine_live_key(v,oldg,1,60,0,0,0));(void)audio(1);
    CHECK(engine_bass_keys_held()&(1u<<v));
    CHECK(engine_live_key(v,newg,2,60,0,0,0));(void)audio(1);
    CHECK(!(engine_bass_keys_held()&(1u<<v)));preserved();
    engine_note_lock_capture(0);
}
static void ui_chord_capture_refusal(int v)
{
    ++cases;phase="unavailable part refuses UI chord capture with NOTE LOCK off/on";
    fresh(v);unavailable_now(v,0);
    ui.part=(uint8_t)(PART_303A+v);ui.spage=0;ui.wpos[v]=7;
    kbd_arp_n[v]=3;kbd_arp_note[v][0]=60;kbd_arp_note[v][1]=64;kbd_arp_note[v][2]=67;
    bass_rec_step[v]=0;
    preserve();uint8_t dirty=ui.dirty;
    for(int lock=0;lock<2;++lock) {
        engine_note_lock_capture(lock);
        bass_write(v,70,G_NOTE);
        CHECK(ui.wpos[v]==7 && bass_rec_step[v]==0 && ui.dirty==dirty);preserved();
        bass_write_held_chord(v);
        CHECK(ui.wpos[v]==7 && bass_rec_step[v]==0 && ui.dirty==dirty);preserved();
        uint8_t oldn=kbd_arp_n[v],oldmask=seq.rec_mask;
        uint8_t oldkeys[UI_BASS_HELD_MAX],oldnotes[UI_BASS_HELD_MAX];
        memcpy(oldkeys,kbd_arp_key[v],sizeof oldkeys);memcpy(oldnotes,kbd_arp_note[v],sizeof oldnotes);
        bass_kbd(4,1);(void)audio(1);
        CHECK(kbd_arp_n[v]==oldn && seq.rec_mask==oldmask);
        CHECK(!memcmp(oldkeys,kbd_arp_key[v],sizeof oldkeys));
        CHECK(!memcmp(oldnotes,kbd_arp_note[v],sizeof oldnotes));
        CHECK(ui.wpos[v]==7 && bass_rec_step[v]==0 && ui.dirty==dirty);preserved();
        CHECK(!(engine_bass_keys_held()&(1u<<v)));
    }
    engine_note_lock_capture(0);kbd_arp_n[v]=0;
}
static void ui_rec_dirty_acknowledgment(int v)
{
    ++cases;phase="UI REC dirty state follows a real accepted recorder write";
    fresh(v);seq_start(&seq);(void)audio(1);
    ui.view=V_SEQ;ui.part=(uint8_t)(PART_303A+v);ui.overlay=O_NONE;ui.rec=1;
    ui_frame();CHECK(seq.rec_mask==(1u<<v));
    ui.dirty=0;preserve();
    CHECK(!seq.rec_dirty);
    bass_kbd(4,1);CHECK(!ui.dirty);
    (void)audio(1);
    CHECK(seq.rec_dirty && seq.rec_bass[v].active);
    CHECK(memcmp(saved_project.pat,proj.pat,sizeof proj.pat)!=0);
    CHECK(!ui.dirty);ui_frame();CHECK(ui.dirty && !seq.rec_dirty);

    phase="queued UI REC refused after actual terminal boundary cannot dirty project";
    fresh(v);seq_start(&seq);(void)audio(1);
    ui.view=V_SEQ;ui.part=(uint8_t)(PART_303A+v);ui.overlay=O_NONE;ui.rec=1;
    ui_frame();CHECK(seq.rec_mask==(1u<<v));
    proj.pat[1].rsv[v]=0xC1u;seq.ppat[TRK_BASS0+v]=1;pattern_sound_tick();
    /* The actual drain budget is 128. A new command behind this exact budget
     * is consumed only after the first render adopts the pending pattern.
     * Same-valued mixer commands neither fake the transition nor edit storage. */
    for(unsigned n=0;n<128;++n)
        engine_set(T_MIX,PART_303A+v,MX_PAN,proj.sound.v[T_MIX][PART_303A+v][MX_PAN]);
    ui.dirty=0;preserve();bass_kbd(4,1);CHECK(!ui.dirty);
    (void)audio(1);CHECK(engine_bass_status(v)==ORIGINAL_RUNTIME_UNAVAILABLE);
    CHECK(!seq.rec_dirty && !seq.rec_bass[v].active);preserved();
    (void)audio(1);CHECK(!seq.rec_dirty && !seq.rec_bass[v].active);preserved();
    ui_frame();CHECK(!ui.dirty && !seq.rec_dirty);preserved();
}
static void real_bar_and_other_slot(int v)
{
    ++cases;phase="real bar adoption and OFF inheritance preserve the other held slot";
    fresh(v);int other=1-v,t=TRK_BASS0+v;
    for(int p=0;p<3;++p)proj.pat[p].bass[v].len=4;
    proj.pat[1].bass[v].step[0]=(bstep_t){52,G_NOTE,110};
    proj.pat[1].rsv[v]=0xC2u;
    midi((unsigned)v,0,60,93);midi((unsigned)other,1,67,93);(void)audio(4);
    CHECK((engine_bass_keys_held()&3)==3);
    uint32_t g,c,og,oc,g1,c1;state(v,&g,&c);state(other,&og,&oc);
    seq_start(&seq);(void)audio(1);seq_cue_part(&seq,t,1);pattern_sound_tick();
    CHECK(seq.ppat[t]==0 && engine_bass_preset_pending(v)==1);
    unsigned n;
    for(n=0;n<2000 && seq.ppat[t]!=1;++n)(void)audio(1);
    CHECK(n<2000 && seq.ppat[t]==1 && seq.t[t].started && eng_step[t]==0);
    CHECK(engine_bass_status(v)==-3 && engine_bass_bank(v)==2 && !engine_bass_preset_pending(v));
    CHECK(!(engine_bass_keys_held()&(1u<<v)) && (engine_bass_keys_held()&(1u<<other)));
    state(v,&g1,&c1);CHECK(g1==g+1 && c1==c+1);
    state(other,&g1,&c1);CHECK(g1==og && c1==oc && engine_bass_status(other)==1);
    seq_cue_part(&seq,t,2);pattern_sound_tick();CHECK(engine_bass_preset_pending(v)==1);
    for(n=0;n<2000 && seq.ppat[t]!=2;++n)(void)audio(1);
    CHECK(n<2000 && seq.ppat[t]==2);
    /* OFF has no onset; the next block's ordinary boundary adopts it. */
    wait_applied(v);CHECK(engine_bass_status(v)==1 && engine_bass_bank(v)==0);
    state(other,&g1,&c1);CHECK(g1==og && c1==oc);
    CHECK(engine_bass_keys_held()&(1u<<other));
    midi((unsigned)other,1,67,0);(void)audio(1);
    CHECK(!(engine_bass_keys_held()&(1u<<other)));
}
static void midi_debt_boundary(int v)
{
    ++cases;phase="actual pattern hard replacement preserves bounded MIDI off debt";
    for(unsigned source=0;source<3;++source) {
        fresh(v);midi((unsigned)v,source,60,93);(void)audio(1);
        CHECK(engine_original_keys(v,0)==1);
        unavailable_now(v,0); /* no transport Stop/panic may erase the debt */
        midi((unsigned)v,source,64,93);(void)audio(1); /* refused unheard press still owes its off */
        CHECK(!(engine_bass_keys_held()&(1u<<v)));
        seq.ppat[TRK_BASS0+v]=2;pattern_sound_tick();(void)audio(1);
        CHECK(engine_bass_status(v)==1 && engine_bass_bank(v)==0);
        for(unsigned key=60;key<=64;key+=4) {
            midi((unsigned)v,source,key,93);(void)audio(1);CHECK(engine_original_keys(v,0)==1);
            midi((unsigned)v,source,key,0);(void)audio(1);CHECK(engine_original_keys(v,0)==1);
            midi((unsigned)v,source,key,0);(void)audio(1);CHECK(engine_original_keys(v,0)==0);
        }
        /* The established 16x256-frame grace resolves a missing second off;
         * it does not promise arbitrary same-pitch MIDI identity. */
        fresh(v);midi((unsigned)v,source,60,93);(void)audio(1);unavailable_now(v,0);
        seq.ppat[TRK_BASS0+v]=2;pattern_sound_tick();(void)audio(1);
        midi((unsigned)v,source,60,93);(void)audio(1); /* tick 1 of the fresh grace */
        midi((unsigned)v,source,60,0);(void)audio(1); /* ambiguous off, tick 2 */
        (void)audio(13);CHECK(engine_original_keys(v,0)==1);
        (void)audio(1);CHECK(engine_original_keys(v,0)==0);
    }
}
static void delayed_off_cache(int v)
{
    ++cases;phase="OFF inspected before delayed Drone adoption must be retried after adoption";
    fresh(v);edited_base(v);int t=TRK_BASS0+v;
    proj.pat[1].rsv[v]=0xC1u;seq.ppat[t]=1;pattern_sound_tick();
    CHECK(engine_bass_preset_pending(v)==1 && engine_bass_preset_gate(v)==1);
    seq.pcue[t]=2;pattern_sound_tick(); /* OFF is inspected while no transient is active yet. */
    CHECK(engine_bass_status(v)==1 && engine_bass_preset_gate(v)==1);
    (void)audio(1);
    CHECK(engine_bass_status(v)==-3 && !engine_bass_preset_pending(v));
    /* A stopped cue becomes actual inside this render. The next main-loop
     * tick must revisit its unchanged OFF byte after the context adopted. */
    CHECK(seq.ppat[t]==2);pattern_sound_tick();
    CHECK(engine_bass_preset_pending(v)==1 && engine_bass_preset_gate(v)==2);
    (void)audio(1);
    CHECK(engine_bass_status(v)==1 && engine_bass_bank(v)==0 && !engine_bass_preset_pending(v));
    base_matches(v,proj.sound.v[T_303][v]);
}
static void rest_starting_bar(int v)
{
    ++cases;phase="REST-starting real bar retires the old owner at the actual boundary";
    for(int exact=0;exact<2;++exact) {
        fresh(v);int t=TRK_BASS0+v;
        for(int p=0;p<2;++p)proj.pat[p].bass[v].len=4;
        midi((unsigned)v,0,60,93);(void)audio(1);CHECK(engine_bass_keys_held()&(1u<<v));
        seq_start(&seq);(void)audio(1);proj.pat[1].rsv[v]=0xC1u;
        uint32_t g,c,g1,c1;state(v,&g,&c);
        seq_cue_part(&seq,t,1);pattern_sound_tick();
        CHECK(seq.ppat[t]==0 && engine_bass_preset_pending(v)==1);
        unsigned count;int exact_edge=0;
        for(count=0;count<2000 && seq.ppat[t]!=1;++count) {
            unsigned n=256;int ending_here=0;
            /* A second run ends the render at the first sample containing the
             * actual pattern event, leaving no extra block to hide a delay. */
            if(exact && seq.t[t].started && seq.t[t].pos==3) {
                unsigned due=(seq.t[t].to_next+255u)>>8;
                if(due && due<=256u){n=due;ending_here=1;}
            }
            frames(n);
            if(seq.ppat[t]==1)exact_edge=ending_here;
        }
        CHECK(count<2000 && seq.ppat[t]==1 && seq.t[t].started && eng_step[t]==0);
        CHECK(!exact || exact_edge);
        CHECK(engine_bass_status(v)==-3 && engine_bass_bank(v)==2);
        CHECK(!engine_bass_preset_pending(v) && !(engine_bass_keys_held()&(1u<<v)));
        state(v,&g1,&c1);CHECK(g1==g+1 && c1==c+1);
        frames(1);CHECK(engine_bass_status(v)==-3 && !(engine_bass_keys_held()&(1u<<v)));
    }
}
static void choose_available(int v,int bank,int factory)
{
    CHECK(engine_bass_select_preset(v,bank,factory,&proj.sound)==0);wait_applied(v);
    CHECK(engine_bass_status(v)==1 && engine_bass_bank(v)==bank && engine_bass_preset_index(v)==factory);
}
static void ordinary_replacement_debt(int v)
{
    ++cases;phase="retained off debt survives ordinary factory and initialized base/history adoption";
    for(int retired=0;retired<2;++retired)for(int path=0;path<3;++path) {
        fresh(v);
        if(retired){midi((unsigned)v,0,60,93);(void)audio(1);CHECK(engine_bass_keys_held()&(1u<<v));}
        unavailable_now(v,0);
        if(!retired){midi((unsigned)v,0,60,93);(void)audio(1);CHECK(!(engine_bass_keys_held()&(1u<<v)));}
        choose_available(v,ORIGINAL_SOUND_JUNO,0);
        if(path==0)choose_available(v,ORIGINAL_SOUND_JUNO,1);
        else {
            /* These are the actual initialized snapshot/history callers; no
             * engine_init, Stop, panic, or synthesized runtime state intervenes. */
            settle();
            CHECK(original_sound_init(proj.sound.v[T_303][v],ORIGINAL_SOUND_JUNO,1));
            engine_bass_sound_refresh(&proj.sound,v);
            if(path==1) {
                bass_base_state_t target;
                for(int b=0;b<NBASS;++b){memcpy(target.cell[b],proj.sound.v[T_303][b],32);target.tag[b]=proj.sound.bass[b];}
                CHECK(engine_bass_base_ready(1u<<v,&target)==0);
                engine_project_hold(1);engine_bass_base_restore(1u<<v);engine_project_hold(0);
            } else engine_apply_sound(&proj.sound);
            (void)audio(1);CHECK(engine_bass_status(v)==1 && engine_bass_preset_index(v)==1);
        }
        midi((unsigned)v,0,60,93);frames(1);CHECK(engine_bass_keys_held()&(1u<<v));
        midi((unsigned)v,0,60,0);frames(1);CHECK(engine_bass_keys_held()&(1u<<v));
        midi((unsigned)v,0,60,0);frames(1);CHECK(!(engine_bass_keys_held()&(1u<<v)));
    }
}
static void link_end_lifetime(int v)
{
    ++cases;phase="actual MIDI source end clears only that unavailable lifetime and preserves the other clock";
    fresh(v);unavailable_now(v,0);
    midi((unsigned)v,0,60,93);midi((unsigned)v,1,67,93);midi((unsigned)v,1,68,93);(void)audio(1);
    choose_available(v,ORIGINAL_SOUND_JUNO,0);
    /* Leave a source-1 timer alive with no current key, then carry its 37-frame
     * fraction across unavailability. Source-1 key67 still owes its old off. */
    midi((unsigned)v,1,68,93);(void)audio(1);
    midi((unsigned)v,1,68,0);(void)audio(1);
    midi((unsigned)v,1,68,0);(void)audio(1);CHECK(!(engine_bass_keys_held()&(1u<<v)));
    proj.pat[1].rsv[v]=0xC2u;pattern_sound_tick();frames(37);CHECK(engine_bass_status(v)==-3);
    min_q[mi_w++%MQ]=0;frames(1); /* actual source-0 link-end packet through midi_in */
    CHECK(engine_bass_status(v)==-3);
    CHECK(engine_bass_select_preset(v,ORIGINAL_SOUND_JUNO,0,&proj.sound)==0);frames(1);
    CHECK(engine_bass_status(v)==1 && !engine_bass_preset_pending(v));
    midi((unsigned)v,0,60,93);frames(1);CHECK(engine_bass_keys_held()&(1u<<v));
    midi((unsigned)v,0,60,0);frames(1);CHECK(!(engine_bass_keys_held()&(1u<<v)));
    midi((unsigned)v,1,67,93);frames(1);CHECK(engine_bass_keys_held()&(1u<<v));
    midi((unsigned)v,1,67,0);frames(1);CHECK(engine_bass_keys_held()&(1u<<v));
    /* Fraction is now43 (37+disconnect+escape+two new-link edges+two other-link
     * edges). Resetting a different source's shared fraction would delay expiry. */
    frames(4096u-43u-1u);CHECK(engine_bass_keys_held()&(1u<<v));
    frames(1);CHECK(!(engine_bass_keys_held()&(1u<<v)));
}
static void manual_choice_stays_superseding(int v)
{
    ++cases;phase="actual adoption epoch must not reassert a raw Drone pattern over an explicit choice";
    fresh(v);unavailable_now(v,3);choose_available(v,ORIGINAL_SOUND_JUNO,0);
    uint32_t serial=engine_bass_choice_serial(),g,c,g1,c1;state(v,&g,&c);
    for(unsigned i=0;i<4;++i) {
        pattern_sound_tick();ui_frame();frames(1);
        CHECK(engine_bass_status(v)==1 && engine_bass_bank(v)==3 && engine_bass_preset_index(v)==0);
        CHECK(!engine_bass_preset_pending(v) && engine_bass_choice_serial()==serial);
        CHECK(proj.pat[1].rsv[v]==0xC4u);state(v,&g1,&c1);CHECK(g1==g && c1==c);
    }
    /* Successful typed restore also clears the unavailable transient; an OFF
     * pattern inspected afterward must not perform another hidden replacement. */
    fresh(v);unavailable_now(v,0);
    CHECK(original_sound_init(proj.sound.v[T_303][v],ORIGINAL_SOUND_JUNO,1));engine_bass_sound_refresh(&proj.sound,v);
    engine_project_hold(1);engine_bass_base_restore(1u<<v);engine_project_hold(0);frames(1);
    CHECK(engine_bass_status(v)==1 && engine_bass_bank(v)==3);
    serial=engine_bass_choice_serial();state(v,&g,&c);
    seq.ppat[TRK_BASS0+v]=2;pattern_sound_tick();frames(1);
    CHECK(engine_bass_choice_serial()==serial && !engine_bass_preset_pending(v));
    state(v,&g1,&c1);CHECK(g1==g && c1==c);
}
#endif

int main(int argc,char **argv)
{
    load_force=0;boot();settle();
    CHECK(!X0X_DRONE_RUNTIME_ENABLED);
#ifndef DRONE_PATTERN_CONTROL_ONLY
    if(argc==2) {
        for(int v=0;v<NBASS;++v) {
            if(!strcmp(argv[1],"off-epoch"))delayed_off_cache(v);
            else if(!strcmp(argv[1],"rest-boundary"))rest_starting_bar(v);
            else if(!strcmp(argv[1],"successive-replace"))ordinary_replacement_debt(v);
            else if(!strcmp(argv[1],"link-end"))link_end_lifetime(v);
            else if(!strcmp(argv[1],"manual-supersede"))manual_choice_stays_superseding(v);
            else return 2;
        }
        printf("Drone unavailable regression %s: %u cases, %u checks PASS\n",argv[1],cases,checks);
        return 0;
    }
#endif
    for(int v=0;v<NBASS;++v) {
        terminal_regression(v);
#ifndef DRONE_PATTERN_CONTROL_ONLY
        for(int factory=0;factory<4;++factory) {
            tested_factory=factory;unavailable_archive(v,factory);archived_base_project(v,factory);
        }
        cancellation_and_forgery(v);available_supersedes(v);
        project_replacement(v);normal_quiescence(v);
        held_key_and_lock(v);ui_chord_capture_refusal(v);ui_rec_dirty_acknowledgment(v);
        real_bar_and_other_slot(v);
        midi_debt_boundary(v);
        delayed_off_cache(v);rest_starting_bar(v);ordinary_replacement_debt(v);
        link_end_lifetime(v);manual_choice_stays_superseding(v);
#endif
    }
    printf("Drone unavailable pattern production integration: %u cases, %u checks PASS\n",cases,checks);
    return 0;
}
