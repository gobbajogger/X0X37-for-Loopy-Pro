/* SPDX-License-Identifier: GPL-3.0-only */
/* Focused #688/#788 port checks against #789's independent mixer context.
 * Production queue, UI, renderer, sequencer and engine; no device I/O. */
#define SMC_UI_MAIN remainder_legacy_main
#include "smc_ui_test.c"
#undef SMC_UI_MAIN

static void tap(int note)
{ message(0x90,note,127); message(0x80,note,0); }
static void render_audio(unsigned n)
{
    int32_t out[512];
    do { unsigned k=n>256?256:n; engine_render(out,k); n-=k; } while(n);
}
static void keyboard_unchanged(screen_state_t before)
{
    screen_state_t after=screen_state();
    CHECK(before.view==after.view && before.part==after.part);
    CHECK(before.mixsel==after.mixsel && before.prev_view==after.prev_view);
    CHECK(!memcmp(before.page,after.page,sizeof before.page));
    CHECK(!memcmp(before.sel,after.sel,sizeof before.sel));
    CHECK(before.overlay==after.overlay && before.help==after.help);
    CHECK(before.clip==after.clip && before.clip_page==after.clip_page);
}

/* Catches fixed MASTER routing instead of the recovered Page-8 cycle. */
static void strip8_modes(void)
{
    setup(V_HOME,PART_303B); screen_state_t keyboard=screen_state();
    tap(7); CHECK(smc.view==V_SONG); tap(7); CHECK(smc.view==V_HOME);
    tap(15); CHECK(smc.view==V_PART); tap(15); CHECK(smc.view==V_SEQ);
    tap(15); CHECK(smc.view==V_GEN); tap(15); CHECK(smc.view==V_PART);
    tap(31); CHECK(smc.view==V_MIX && smc_page_now()==0);
    CHECK(!strcmp(smc_button_caption(31),"MIX"));
    tap(31); CHECK(smc_page_now()==1 && smc_drum_mix()==PART_909);
    /* Populated drum strip 8 owns its sound; empty bank-2 strips stay inert. */
    tap(31); CHECK(smc.sel[PART_909]==7 && smc_page_now()==1);
    tap(31); CHECK(smc_page_now()==0);
    smc_page_set(1); tap(0x2f); CHECK(smc.sel[PART_909]==8);
    pref_t target=smc_target(0); tap(31);
    CHECK(smc_page_now()==1 && smc.sel[PART_909]==8);
    CHECK(smc_same(target,smc_target(0)) && smc_target(7).kind==R_NONE);
    keyboard_unchanged(keyboard);
}

/* Catches Up/Down changing a clip stage or strip-8 changing the wrong stage. */
static void clip_stage_and_banks(void)
{
    setup(V_HOME,PART_303B); clip_grid_mode=1; ck.pg=0;
    CHECK(!strcmp(smc_button_caption(7),"NEXT"));
    CHECK(!strcmp(smc_button_caption(15),"PREV"));
    CHECK(!strcmp(smc_button_caption(31),"NEXT"));
    CHECK(!strcmp(smc_button_caption(23),"M"));
    tap(7); CHECK((ck.pg&3u)==1); tap(7); CHECK((ck.pg&3u)==2);
    tap(15); CHECK((ck.pg&3u)==1); tap(31); CHECK((ck.pg&3u)==2);
    screen_state_t keyboard=screen_state();
    tap(0x61); CHECK(smc.knob_bank==1); keyboard_unchanged(keyboard);
    tap(0x60); CHECK(smc.knob_bank==0); keyboard_unchanged(keyboard);
    ui.overlay=O_ASK; tap(7); CHECK((ck.pg&3u)==2);
    ui.overlay=O_NONE; clip_grid_mode=0; tap(0x5d); CHECK(smc.locked);
    clip_grid_mode=1; tap(7); CHECK((ck.pg&3u)==2);
    CHECK(strcmp(smc_button_caption(7),"NEXT"));
}

/* A populated drum strip must not reopen a REC/SOLO card after clip navigation. */
static void clip_stage_drum_contexts(void)
{
    const uint8_t notes[]={7,15,31};
    for(int kit=0;kit<NKIT;kit++) for(int v=0;v<2;v++) {
        for(unsigned n=0;n<sizeof notes;n++) {
            setup(V_HOME,PART_303B);
            smc.view=v?V_SEQ:V_MIX; smc.part=smc.mixsel=(uint8_t)kit;
            smc.sel[kit]=0;
            if(!v) smc_page_set(kit+1);
            CHECK(smc_drum_mix()==kit && smc_drum_track(kit,7)==7);
            fader(0,0); CHECK(smc_ui_active());
            clip_grid_mode=1; ck.pg=1;
            screen_state_t keyboard=screen_state();
            uint8_t rec=ui.rec, rec_mask=seq.rec_ext_mask, rec_part=smc.rec_part;
            uint8_t page=smc_page_now(), bank=smc.knob_bank;
            uint8_t note=notes[n];
            CHECK(!strcmp(smc_button_caption(note),note==15?"PREV":"NEXT"));
            message(0x90,note,127);
            CHECK((ck.pg&3u)==(note==15?0u:2u));
            CHECK(!smc_ui_active());
            CHECK(ui.rec==rec && seq.rec_ext_mask==rec_mask && smc.rec_part==rec_part);
            CHECK(!smc.solo && !smc.drum_solo[0] && !smc.drum_solo[1]);
            CHECK(!smc.solo_target[7]); /* Check before release can clear a transient Solo. */
            CHECK(smc.view==(v?V_SEQ:V_MIX) && smc.part==kit && smc.sel[kit]==0);
            CHECK(smc_page_now()==page && smc.knob_bank==bank);
            keyboard.clip_page=ck.pg; keyboard_unchanged(keyboard);
            message(0x80,note,0);
            CHECK(!smc_ui_active() && (ck.pg&3u)==(note==15?0u:2u));
            CHECK(ui.rec==rec && seq.rec_ext_mask==rec_mask && smc.rec_part==rec_part);
            CHECK(!smc.solo && !smc.drum_solo[0] && !smc.drum_solo[1]);
            CHECK(!smc.solo_target[7]); keyboard_unchanged(keyboard);
        }
    }
}

/* A cycle is navigation; it cannot reinterpret a captured PART page as MIX. */
static void locked_navigation_and_fx(void)
{
    setup(V_PART,PART_303B); tap(0x5d); CHECK(smc.locked);
    uint8_t locked_page=smc.lock_page; pref_t target=smc_target(0);
    pref_t control=smc_knob_target(0); screen_state_t keyboard=screen_state();
    tap(31); CHECK(smc.view==V_MIX && smc.mixer_page[V_MIX][0]==0);
    tap(31); CHECK(smc.mixer_page[V_MIX][0]==1);
    CHECK(smc.lock_view==V_PART && smc.lock_page==locked_page);
    CHECK(smc_same(target,smc_target(0)) && smc_same(control,smc_knob_target(0)));
    keyboard_unchanged(keyboard);
    setup(V_MIX,PART_303B); smc_page_set(3); tap(0x5d);
    target=smc_target(0); control=smc_knob_target(0);
    tap(7); CHECK(smc.view==V_HOME); tap(31);
    CHECK(smc.view==V_MIX && smc.mixer_page[V_MIX][0]==0);
    CHECK(smc.lock_view==V_MIX && smc.lock_page==3);
    CHECK(smc_same(target,smc_target(0)) && smc_same(control,smc_knob_target(0)));
    setup(V_FX,PART_303B); uint8_t bank=smc.knob_bank;
    message(0x90,6,127); tap(0x60); tap(0x61); message(0x80,6,0);
    CHECK(smc.knob_bank==bank); /* HOME modifiers retain cutoff save/reset. */
    tap(31); CHECK(smc.view==V_MIX && smc_page_now()==3); /* FX short tap owns MASTER. */
    setup(V_FX,PART_303B);
    message(0x90,24,127); now_ms+=SMC_FX_HOLD_MS; smc_poll();
    CHECK(smc.fx_down & 1u); CHECK(smc.view==V_FX);
    message(0x80,24,0); CHECK(!(smc.fx_down & 1u)); CHECK(smc.view==V_FX);
}

/* Use both real keyboard header paths without replacing the transport state. */
static void render_clock_header(int part_header, unsigned frame)
{
    smc_ui_cancel(); ui.overlay=O_NONE; ui.help=0; ui.touched=-1;
    ui.part=PART_303B; set_view(part_header?V_PART:V_HOME); build_pages();
    ui_frame(); /* Let the real WAIT transition publish its one-time message. */
    ui.msg[0]=0; ui.msg_until=0; ui.frame=frame;
    ui_frame();
}
static void check_clock_headers(uint16_t expected, unsigned frame)
{
    for(int p=0;p<2;p++) {
        render_clock_header(p,frame);
        CHECK(fb[9*240+(p?176:218)]==(uint16_t)((expected<<8)|(expected>>8)));
    }
}

/* Catches track-step-driven icons, missing Part-header icon and wall-time blink. */
static void clock_icons(void)
{
    const uint8_t phases[]={0,1,4,5,16,17,32};
    for(int part_header=0;part_header<2;part_header++) {
        setup(part_header?V_PART:V_HOME,PART_303B); seq.playing=1;
        for(unsigned i=0;i<sizeof phases;i++) {
            seq.metro_view=phases[i]; ui_frame();
            uint16_t c=!phases[i]?C_DIM:phases[i]<=4?C_WHITE:C_HI;
            CHECK(fb[9*240+(part_header?176:218)]==(uint16_t)((c<<8)|(c>>8)));
            if(phases[i]) CHECK(smc_led(0x5d)==(phases[i]<=16));
        }
        seq.metro_view=1; eng_step[TRK_DRUM]=0; draw_header();
        memcpy(before_fb,fb,sizeof fb); eng_step[TRK_DRUM]=3; draw_header();
        CHECK(!memcmp(before_fb,fb,sizeof fb));
    }
    setup(V_HOME,PART_303B); seq.bpm=120; seq.clk_src=SEQ_CLK_INT;
    pref_set(PR(R_CLKSRC,0,0,0),SEQ_CLK_INT);
    seq.send_clock=0; render_audio(1); CHECK(!seq.metro_view);
    check_clock_headers(C_DIM,0);
    seq_start(&seq); render_audio(1); CHECK(seq.metro_view==1);
    check_clock_headers(C_WHITE,0);
    fader(0,0);
    smc_ui_draw_peer(); memcpy(before_fb,fb,sizeof fb);
    render_audio(FS/4); CHECK(seq.metro_view>=16);
    smc_ui_draw_peer(); CHECK(changed_rows(MAIN_Y,MAIN_Y+2*BAND_H,before_fb,fb)>0);
    unsigned phase=seq.metro_view; seq_stop(&seq); render_audio(1); CHECK(!seq.metro_view);
    check_clock_headers(C_DIM,0);
    render_audio(FS); seq_continue(&seq); render_audio(1); CHECK(seq.metro_view==phase);
    check_clock_headers(C_HI,0); /* Continue preserves the elapsed beat phase. */
    seq_stop(&seq); render_audio(1);
    check_clock_headers(C_DIM,0);
    pref_set(PR(R_CLKSRC,0,0,0),SEQ_CLK_EXT); seq.clk_src=SEQ_CLK_EXT;
    seq_start(&seq); render_audio(1); CHECK(!seq.metro_view);
    CHECK(seq_clock_waiting(&seq));
    check_clock_headers(C_DIM,0); check_clock_headers(C_DIM,32);
    seq_midi_rt(&seq,0xf8); render_audio(1); CHECK(seq.metro_view==1);
    CHECK(!seq_clock_waiting(&seq)); check_clock_headers(C_WHITE,0);
    render_audio(FS); CHECK(!seq.metro_view);
    CHECK(seq_clock_waiting(&seq)); check_clock_headers(C_DIM,0);
}

/* Retained rows: deliberate drum selection, pending cue cancellation and hold. */
static void retained_contracts(void)
{
    for(int kit=0;kit<NKIT;kit++) {
        setup(V_HOME,PART_BRK); screen_state_t keyboard=screen_state();
        tap(24+part_position(kit)); CHECK(smc.part==kit && smc.view==V_PART);
        tap(24+part_position(kit)); CHECK(smc.view==V_MIX && smc_drum_mix()==kit);
        keyboard_unchanged(keyboard);
    }
    setup(V_HOME,PART_303B); seq.playing=1; seq.ppat[PART_303B]=2;
    tap(0x5c); CHECK(seq_cue_of(&seq,PART_303B)==3);
    tap(0x5c); CHECK(seq_cue_of(&seq,PART_303B)==4);
    tap(0x5b); CHECK(seq_cue_of(&seq,PART_303B)==3);
    tap(0x5b); CHECK(seq_cue_of(&seq,PART_303B)<0);
    fader(0,0); message(0x90,16,127); CHECK(smc_ui_active());
    now_ms+=SMC_UI_MS+1; CHECK(smc_ui_active());
    message(0x80,16,0); now_ms+=SMC_UI_MS-1; CHECK(smc_ui_active());
    now_ms+=2; CHECK(!smc_ui_active());
}

/* Dedicated capture driver calls this on BOTH pins, separately from assertions. */
static void capture_scenes(void)
{
    for(int p=0;p<2;p++) {
        setup(p?V_PART:V_HOME,PART_303B); seq.playing=1;
        seq.metro_view=1; ui_frame(); shot(p?"part-beat-lit":"header-beat-lit");
        seq.metro_view=17; ui_frame(); shot(p?"part-beat-dim":"header-beat-dim");
        seq_stop(&seq); render_audio(1); render_clock_header(p,0);
        shot(p?"part-stop-neutral":"header-stop-neutral");
        pref_set(PR(R_CLKSRC,0,0,0),SEQ_CLK_EXT); seq.clk_src=SEQ_CLK_EXT;
        seq_start(&seq); render_audio(1); render_clock_header(p,0);
        shot(p?"part-wait-neutral":"header-wait-neutral");
        render_clock_header(p,32);
        shot(p?"part-wait-neutral-blink":"header-wait-neutral-blink");
    }
    setup(V_HOME,PART_303B); fader(0,0);
    seq.metro_view=1; smc_ui_draw_peer(); shot("pickup-lit");
    seq.metro_view=17; smc_ui_draw_peer(); shot("pickup-dim");
    setup(V_HOME,PART_303B); clip_grid_mode=1; fader(0,0);
    smc_ui_draw_peer(); shot("clip-strip8");
    tap(0x61); smc_ui_draw_peer(); shot("clip-knob-bank");
    setup(V_HOME,PART_303B); tap(31); fader(0,0);
    smc_ui_draw_peer(); shot("select8-mix");
}

#ifndef F4_CAPTURE_DRIVER
int main(void)
{
    CHECK(X0X_ORIGINAL_SYNTH==1 && PROJ_SND_REV==13);
    strip8_modes(); clip_stage_and_banks(); clip_stage_drum_contexts(); locked_navigation_and_fx();
    clock_icons(); retained_contracts();
    printf("smc-remainder: %u checks PASS\n",checks);
    return 0;
}
#endif
