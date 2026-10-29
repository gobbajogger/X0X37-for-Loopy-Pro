/* SPDX-License-Identifier: GPL-3.0-only */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/original_sound.h"

static unsigned checks;
static uint64_t audio_hash = UINT64_C(1469598103934665603);
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static uint32_t float_bits(float x) { uint32_t bits; memcpy(&bits, &x, 4); return bits; }
static void block(unsigned frames)
{
    int32_t audio[514];
    for (unsigned i = 0; i < 514; ++i) audio[i] = 0x12345678;
    engine_render(audio, frames);
    CHECK(audio[frames * 2] == 0x12345678);
    CHECK(audio[frames * 2 + 1] == 0x12345678);
}
static void settle(void) { for (unsigned i = 0; i < 12; ++i) block(256); }
static void midi(unsigned source, unsigned status, unsigned key, unsigned velocity)
{ min_q[mi_w++ % MQ] = (status >> 4) | source << 4 | status << 8 | key << 16 | velocity << 24; }
static void select_miami(int v, unsigned factory)
{
    CHECK(engine_bass_select_preset(v, ORIGINAL_SOUND_MIAMI, (int)factory, &proj.sound) == 0);
    settle();
    /* A source-free voice can still own an audible insert tail. Wait for the
     * measured quiet boundary rather than assuming twelve blocks is enough. */
    for(unsigned i=0;i<4096 && engine_bass_preset_pending(v);i++) block(256);
    CHECK(engine_bass_preset_pending(v) == 0);
    CHECK(engine_bass_status(v) == 1 && engine_bass_bank(v) == ORIGINAL_SOUND_MIAMI);
    CHECK(engine_bass_preset_index(v) == (int)factory);
    CHECK(!engine_bass_preset_edited(v));
    for (unsigned p = 0; p < 18; ++p) {
        float value;
        CHECK(original_sound_value(proj.sound.v[T_303][v], p, &value));
        CHECK(engine_original_value_bits(v, p) == float_bits(value));
    }
}
static void select_chicago(int v, unsigned factory)
{
    CHECK(engine_bass_select_preset(v, ORIGINAL_SOUND_CHICAGO, (int)factory, &proj.sound) == 0);
    settle();
    /* A source-free voice can still own an audible insert tail. Wait for the
     * measured quiet boundary rather than assuming twelve blocks is enough. */
    for(unsigned i=0;i<4096 && engine_bass_preset_pending(v);i++) block(256);
    CHECK(engine_bass_preset_pending(v) == 0);
    CHECK(engine_bass_status(v) == 1 && engine_bass_bank(v) == ORIGINAL_SOUND_CHICAGO);
    CHECK(engine_bass_preset_index(v) == (int)factory && !engine_bass_preset_edited(v));
    for (unsigned p = 0; p < 22; ++p) {
        float value;
        CHECK(original_sound_value(proj.sound.v[T_303][v], p, &value));
        CHECK(engine_original_value_bits(v, p) == float_bits(value));
    }
}
static void cold(void)
{
    engine_project_hold(1);
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    engine_apply_sound(&proj.sound);
    seq.ch[PART_909] = 9; seq.ch[PART_808] = 10; seq.ch[PART_BRK] = 11;
    seq.ch[PART_303A] = 2; seq.ch[PART_303B] = 3;
    engine_project_release();
    settle();
}

static void compact_audio(void)
{
    static const unsigned macro[7] = {0,5,7,4,2,3,1};
    /* A fresh graph for each part isolates its audible output. The other part
     * receives no note. All seven choices pass through the real DSP/mixer. */
    for(unsigned kind=0;kind<2;++kind) {
      if(kind==ORIGINAL_SOUND_CHICAGO) {select_chicago(0,0); select_chicago(1,0);}
      else {select_miami(0,0); select_miami(1,0);}
      for (unsigned voice=0;voice<2;++voice) for (unsigned choice=0;choice<(kind?1u:7u);++choice) {
        cold();
        engine_guard_enable(0);
        if(kind==ORIGINAL_SOUND_CHICAGO) {
            engine_set(T_303,(int)voice,17,(int)choice);
            block(256);
            CHECK(engine_original_value_bits((int)voice,17)==float_bits((float)macro[choice]/11.0f));
            uint32_t valid=engine_original_value_bits((int)voice,17);
            engine_set(T_303,(int)voice,17,7); engine_set(T_303,(int)voice,17,127); block(256);
            CHECK(engine_bass_status((int)voice)==1 && engine_original_value_bits((int)voice,17)==valid);
        }
        midi(0,0x92u+voice,60,100);
        unsigned nonzero[2]={0,0};
        for(unsigned b=0;b<24;++b) {
            int32_t audio[514];
            audio[512]=audio[513]=0x12345678;
            engine_render(audio,256); engine_volume(audio,256);
            CHECK(audio[512]==0x12345678 && audio[513]==0x12345678);
            for(unsigned i=0;i<512;++i) {
                CHECK(audio[i]>=-8388608 && audio[i]<=8388607);
                nonzero[i&1u]+=audio[i]!=0;
                audio_hash^=(uint32_t)audio[i]; audio_hash*=UINT64_C(1099511628211);
            }
        }
        CHECK(nonzero[0] && nonzero[1]);
        CHECK(engine_original_keys((int)voice,0)==1 && engine_original_keys(1-(int)voice,0)==0);
        midi(0,0x82u+voice,60,0); block(256);
        CHECK(engine_original_keys((int)voice,0)==0 && engine_bass_status((int)voice)==1);
      }
    }
    /* Two independently selected histories are live at the same time. */
    select_chicago(0,0); select_chicago(1,0);
    cold();
    engine_set(T_303,0,17,1); engine_set(T_303,1,17,2);
    midi(0,0x92,60,100); midi(0,0x93,67,90); block(256);
    CHECK(engine_original_keys(0,0)==1 && engine_original_keys(1,0)==1);
    midi(0,0x82,60,0); block(256);
    CHECK(engine_original_keys(0,0)==0 && engine_original_keys(1,0)==1);
    midi(0,0x83,67,0); block(256);
    CHECK(engine_bass_status(0)==1 && engine_bass_status(1)==1);
    cold();
}

static void compact_ui(const char *directory)
{
    static const char *const names[7]={"OFF","CHO/FLG","PHASER","RING MOD","COMPRESSOR","DECIMATOR","EQ"};
    pref_t effect=PR(R_ENG,T_303,1,17), preset=PR(R_BPRESET,0,1,0);
    select_chicago(1,0);
    ui.view=V_PART; ui.part=PART_303B; ui.overlay=0; ui.help=0;
    ui.msg_until=ui.touch_until=0; ui.touched=-1;
    build_pages();
    int page=-1;
    for(int p=0;p<pg.n;++p)
        if(pg.r[p][0].kind==R_ENG && pg.r[p][0].c==17) page=p;
    CHECK(page==6 && pref_desc(effect)->max==6 && pref_desc(preset)->max==54);
    for(unsigned i=0;i<7;++i) {
        pref_set(effect,(int)i); block(256);
        char num[32],unit[16]; pref_value_of(effect,pref_get(effect),num,unit);
        CHECK(!strcmp(num,names[i]) && !*unit);
        uint8_t before[32]; memcpy(before,proj.sound.v[T_303][1],32);
        ui.page[V_PART][PART_303B]=(uint8_t)page;
        ui_frame();
        CHECK(!memcmp(before,proj.sound.v[T_303][1],32));
        if(directory) {
            char path[512];
            CHECK(snprintf(path,sizeof path,"%s/chicago-fx-%u.png",directory,i)>0);
            png_write(path);
        }
    }
    pref_set(preset,54); settle();
    CHECK(engine_bass_preset_index(1)==54 && !strcmp(original_sound_name(0,54),"Strome FX"));
    pref_set(preset,0); settle();
    CHECK(engine_bass_preset_index(1)==0 && !strcmp(original_sound_name(0,0),"Bite Bass"));
}

static void compact_project(void)
{
    select_chicago(1,0);
    engine_project_hold(1);
    project_capture_stage();
    CHECK(project_validate_stage()==0 && project_stage.set.snd_rev==13 && PROJ_STREAM==13);
    project_stage.sound.v[T_303][1][17]=7;
    CHECK(project_validate_stage()!=0);
    project_capture_stage(); project_stage.sound.v[T_303][1][28]=1;
    CHECK(project_validate_stage()!=0);
    project_capture_stage(); project_stage.set.snd_rev=12;
    CHECK(project_validate_stage()!=0);
    project_capture_stage();
    int lane=motion_alloc(project_stage.arr.lane,0,PART_303B,T_303,1,17);
    CHECK(lane>=0);
    project_stage.arr.lane[lane].val[0]=7;
    CHECK(project_validate_stage()!=0);
    project_stage.arr.lane[lane].val[0]=6;
    CHECK(project_validate_stage()==0);
    CHECK(project_save()==0 && project_load_stage()==0);
    CHECK(!memcmp(&proj.sound,&project_stage.sound,sizeof proj.sound));
    engine_project_release();
}

int main(int argc, char **argv)
{
    load_force = 0;
    boot(); settle();
    CHECK(engine_bass_sound_validate(&proj.sound) == 0);
    CHECK(engine_bass_status(0) == 1);
    CHECK(engine_bass_status(1) == 1);
    CHECK(engine_bass_bank_available(1, ORIGINAL_SOUND_CHICAGO));
    CHECK(engine_nparams(T_303, 0) == 18 && engine_nparams(T_303, 1) == 22);
    CHECK(engine_bass_select_preset(1, ORIGINAL_SOUND_CHICAGO, 55, &proj.sound) == -1);
    for(unsigned factory=0;factory<55;++factory) {
        select_chicago(0,factory); select_chicago(1,54-factory);
    }
    compact_audio();
    compact_ui(argc==2?argv[1]:NULL);
    compact_project();
    select_miami(1, 0);
    for (unsigned factory = 0; factory < 30; ++factory) {
        select_miami(0, factory);
        select_miami(1, 29 - factory);
    }
    select_miami(0, 0); select_miami(1, 0); cold();

    /* Old integration dropped releases unless they matched its latest key. */
    midi(0, 0x92, 60, 100); midi(0, 0x92, 64, 90); block(31);
    CHECK(engine_original_keys(0, 0) == 2);
    midi(0, 0x82, 60, 0); block(17); CHECK(engine_original_keys(0, 0) == 1);
    midi(0, 0x82, 64, 0); block(1); CHECK(engine_original_keys(0, 0) == 0);
    midi(0, 0x92, 0, 127); block(256); CHECK(engine_original_keys(0, 0) == 1);
    midi(0, 0x82, 0, 0); block(256); CHECK(engine_original_keys(0, 0) == 0);

    /* Duplicate MIDI edges and independent sources never lose the final off. */
    midi(0, 0x92, 60, 100); midi(0, 0x92, 60, 110); midi(1, 0x92, 60, 90); block(256);
    CHECK(engine_original_keys(0, 0) == 1);
    midi(0, 0x82, 60, 0); midi(1, 0x82, 60, 0); block(256);
    CHECK(engine_original_keys(0, 0) == 1);
    midi(0, 0x82, 60, 0); block(256); CHECK(engine_original_keys(0, 0) == 0);
    midi(0, 0x92, 60, 100); midi(1, 0x92, 64, 100); block(256);
    min_q[mi_w++ % MQ] = PLAT_MIDI_LINK_END(0); block(256);
    CHECK(engine_original_keys(0, 0) == 1);
    seq.ch[PART_303A] = 4; block(256); CHECK(engine_original_keys(0, 0) == 0);
    midi(1, 0x94, 67, 100); block(256);
    CHECK(engine_original_keys(0,0)==1); /* the new channel accepts a fresh note after retirement */
    midi(1, 0x84, 67, 0); block(256);
    seq.ch[PART_303A] = 2; block(256);

    uint32_t generation, context; int arp;
    bass_base_state_t base_candidate;
    for (unsigned v=0;v<NBASS;v++) {
        memcpy(base_candidate.cell[v],proj.sound.v[T_303][v],NPARAMS_MAX);
        base_candidate.tag[v]=proj.sound.bass[v];
    }
    engine_live_key_state(0, &generation, &arp, &context);
    CHECK(engine_live_key(0, generation, 1, 60, 100, 0, 0));
    CHECK(engine_project_notes()); /* accepted FIFO strike blocks project transfer before audio */
    CHECK(engine_bass_base_ready(1,&base_candidate)==2); /* Undo must wait for that accepted strike */
    CHECK(engine_live_key(0, generation, 2, 64, 100, 0, 0)); block(256);
    CHECK(engine_original_keys(0, 0) == 2);
    CHECK(engine_live_key(0, generation, 1, 60, 0, 0, 0)); block(256);
    CHECK(engine_original_keys(0, 0) == 1);
    CHECK(engine_live_key(0, generation, 2, 64, 0, 0, 0)); block(256);
    CHECK(engine_original_keys(0, 0) == 0);
    /* Two physical tokens on one pitch release only after both keys. */
    CHECK(engine_live_key(0, generation, 3, 67, 100, 0, 0));
    CHECK(engine_live_key(0, generation, 4, 67, 100, 0, 0)); block(256);
    CHECK(engine_live_key(0, generation, 3, 67, 0, 0, 0)); block(256);
    CHECK(engine_original_keys(0, 0) == 1);
    engine_live_key_cancel(0); block(256); CHECK(engine_original_keys(0, 0) == 0);

    /* A temporary lock/base restore preserves a factory float off the UI grid. */
    unsigned parameter = 0; float exact = 0;
    for (; parameter < 18; ++parameter) {
        CHECK(original_sound_value(proj.sound.v[T_303][0], parameter, &exact));
        if (float_bits(exact) != float_bits((float)proj.sound.v[T_303][0][parameter] / 127.0f)) break;
    }
    CHECK(parameter < 18);
    engine_set(T_303, 0, (int)parameter, 127); block(256);
    CHECK(engine_original_value_bits(0, parameter) == float_bits(1.0f));
    engine_set_base(T_303, 0, (int)parameter); block(256);
    CHECK(engine_original_value_bits(0, parameter) == float_bits(exact));
    int lane = motion_alloc(proj.arr.lane, 0, PART_303A, T_303, 0, (int)parameter);
    CHECK(lane >= 0);
    engine_set(T_303, 0, (int)parameter, 0); block(256);
    engine_motion_clear(lane);
    CHECK(engine_original_value_bits(0, parameter) == float_bits(0.0f)); /* main loop must not mutate DSP */
    block(256);
    CHECK(engine_original_value_bits(0, parameter) == float_bits(exact));
    CHECK(!original_sound_edit_mask(proj.sound.v[T_303][0]));

    /* Explicit edits survive a complete fresh engine graph. */
    CHECK(original_sound_set(proj.sound.v[T_303][0], parameter, 41));
    engine_bass_sound_refresh(&proj.sound, 0);
    CHECK(engine_bass_sound_validate(&proj.sound) == 0);
    cold();
    CHECK(engine_original_value_bits(0, parameter) == float_bits(41.0f / 127.0f));
    midi(0, 0xb2, 64, 127); midi(0, 0x92, 60, 100); midi(0, 0x82, 60, 0); block(256);
    CHECK(engine_original_keys(0, 0) == 0 && engine_original_keys(0, 1) == 1);
    midi(0, 0xb2, 64, 0); block(256);
    CHECK(engine_original_value_bits(0, 17) == float_bits(0.0f));
    CHECK(engine_original_keys(0, 0) == 0);
    /* Original ARP-off state retains its latched array as dormant metadata.
     * A pedal-up release must not rewrite it merely to satisfy an idle check. */
    CHECK(engine_original_keys(0, 1) == 1);
    midi(0, 0xe2, 0, 64); block(256);
    CHECK(engine_original_value_bits(0, 16) == float_bits(0.5f));
    seq_start(&seq); block(256);
    engine_set(T_303, 0, 17, 127);
    engine_bass_on_velocity(0, 72, 0, 0, 100); block(256);
    CHECK(engine_original_keys(0, 0));
    seq_stop(&seq); block(256);
    CHECK(!seq.playing && !engine_original_keys(0, 0) && !engine_original_keys(0, 1));
    /* A note following Stop in the same drain belongs to stopped free play;
     * a late block-end transport callback must never erase that newer note. */
    seq_start(&seq); block(256); seq_stop(&seq);
    midi(0, 0x92, 75, 100); block(256);
    CHECK(!seq.playing && engine_original_keys(0, 0) == 1);
    midi(0, 0x82, 75, 0); midi(0, 0xb2, 64, 0); block(256);

    /* The release recorder/manual-note queue has the same Stop ordering. */
    seq_start(&seq); block(256); seq_stop(&seq);
    engine_bass_on_velocity(0, 77, 0, 0, 100); block(256);
    CHECK(!seq.playing && engine_original_keys(0, 0) == 1);
    engine_bass_off_note(0, 77); block(256);
    CHECK(engine_original_keys(0, 0) == 0);

    /* Physical input accepted after Stop shares the same ordering guarantee.
     * Retire old sounding owners without revoking these queued new presses. */
    seq_start(&seq); block(256);
    engine_live_key_state(0, &generation, &arp, &context);
    CHECK(engine_live_key(0, generation, 1, 60, 100, 0, 0)); block(256);
    seq_stop(&seq);
    CHECK(engine_live_key(0, generation, 2, 64, 100, 0, 0)); block(256);
    CHECK(!seq.playing && engine_original_keys(0, 0) == 1);
    CHECK(engine_live_key(0, generation, 1, 60, 0, 0, 0)); block(256);
    CHECK(engine_original_keys(0, 0) == 1); /* old release cannot kill the new key */
    CHECK(engine_live_key(0, generation, 2, 64, 0, 0, 0)); block(256);
    CHECK(engine_original_keys(0, 0) == 0);

    /* MIDI Stop at the ring tail must precede physical FIFO consumption. */
    seq.clk_src = SEQ_CLK_AUTO; seq.clk_lock = 0;
    seq_start(&seq); block(256);
    CHECK(engine_live_key(0, generation, 3, 67, 100, 0, 0)); block(256);
    midi(0, 0xfc, 0, 0);
    CHECK(engine_live_key(0, generation, 4, 72, 100, 0, 0)); block(256);
    CHECK(!seq.playing && engine_original_keys(0, 0) == 1);
    CHECK(engine_live_key(0, generation, 3, 67, 0, 0, 0)); block(256);
    CHECK(engine_original_keys(0, 0) == 1);
    CHECK(engine_live_key(0, generation, 4, 72, 0, 0, 0)); block(256);
    CHECK(engine_original_keys(0, 0) == 0);
    /* Explicit cancel retains its stricter context/admission retirement. */
    engine_live_key_cancel(0);
    CHECK(engine_live_key(0, generation, 5, 76, 100, 0, 0)); block(256);
    CHECK(engine_original_keys(0, 0) == 0);
    uint32_t next_generation;
    engine_live_key_state(0, &next_generation, &arp, &context);
    CHECK(next_generation != generation);
    CHECK(engine_bass_status(0) == 1 && engine_bass_status(1) == 1);
    printf("PASS compact firmware integration: %u checks;55 Chicago/30 Miami factories in each independent part; seven Chicago FX and Miami audibly rendered on each part; two simultaneous Chicago owners; named UI, schema2/revision13/project roundtrip; MIDI/all-key releases, source/channel retirement, physical tokens, exact base/motion restore, sustain/bend/Stop, guarded output; audio hash%016llx\n",checks,(unsigned long long)audio_hash);
    return 0;
}
