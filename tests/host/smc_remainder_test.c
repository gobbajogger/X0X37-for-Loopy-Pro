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

#ifndef F4_CAPTURE_DRIVER
#include "app/smk_arrows.h"

/* Catches page/SELECT routing stealing the edit chord or rewriting adjacent cells. */
static void held_fx_arrows(void)
{
    setup(V_FX,PART_303B); smc_fx_load_page(0);
    uint8_t original[sizeof smc.fx_strip.map];
    memcpy(original,smc.fx_strip.map,sizeof original);
    screen_state_t keyboard=screen_state();
    message(0x90,0,127); CHECK(smc.fx_down==1u);
    CHECK(smcfx_strip_live_fx(&smc.fx_strip,0)==SMCFX_REV);
    uint8_t page=smc_page_now(), bank=smc.knob_bank;
    tap(0x63); CHECK(smc.fx_strip.map[0][0][0]==SMCFX_DLY);
    CHECK(smcfx_strip_live_fx(&smc.fx_strip,0)==SMCFX_DLY);
    CHECK(smc_page_now()==page && smc.knob_bank==bank);
    for(unsigned i=1;i<sizeof original;i++) CHECK(((uint8_t*)smc.fx_strip.map)[i]==original[i]);
    tap(0x62); CHECK(smc.fx_strip.map[0][0][0]==SMCFX_REV);
    tap(0x62); CHECK(smc.fx_strip.map[0][0][0]==SMCFX_SHUF); /* backwards wrap */
    arw_t arrows={0};
    CHECK(arw_frame(&arrows,2,ARW_SELECT,now_ms)==0); /* no generated SELECT detent */
    CHECK(smc.fx_strip.map[0][0][0]==SMCFX_REV); /* forward wrap */
    CHECK(arw_frame(&arrows,2,ARW_MENU,now_ms+1000)==0);
    CHECK(smc.fx_strip.map[0][0][0]==SMCFX_REV && !arrows.rep); /* one step per press */
    keyboard_unchanged(keyboard);
    message(0x80,0,0); CHECK(!smc.fx_down);
    CHECK(arw_frame(&arrows,0,ARW_SELECT,now_ms)==0);
    CHECK(arw_frame(&arrows,2,ARW_SELECT,now_ms)==1); /* ordinary arrows restored */

    /* An empty page-two slot owns its original page and stays silent until edited. */
    setup(V_FX,PART_303B); smc_fx_load_page(1);
    message(0x90,0,127); CHECK(!smc.fx_down);
    CHECK(smc.fx_strip.key_bus[0]==0);
    smc_fx_load_page(0); /* a separate page change cannot redirect the held edit */
    tap(0x62); CHECK(smc.fx_strip.map[1][0][0]==SMCFX_SHUF);
    CHECK(smc.fx_strip.map[0][0][0]==SMCFX_REV && smc.fx_down==1u);
    message(0x80,0,0); CHECK(!smc.fx_down);
    setup(V_FX,PART_303B); smc_fx_load_page(1);
    message(0x90,0,127); tap(0x63);
    CHECK(smc.fx_strip.map[1][0][0]==SMCFX_REV); message(0x80,0,0);

    /* HOME no longer assigns FX. Multiple held keys consume arrows without editing. */
    setup(V_FX,PART_303B); held_btn=1u<<B_HOME;
    message(0x90,0,127); CHECK(smc.fx_down==1u && smc.fx_strip.map[0][0][0]==SMCFX_REV);
    CHECK(!strcmp(smc_button_caption(24),"SHUF")); /* HOME must not advertise removed CYC remapping. */
    message(0x80,0,0); held_btn=0;
    message(0x90,0,127); message(0x90,8,127);
    memcpy(original,smc.fx_strip.map,sizeof original); page=smc_page_now();
    tap(0x63); CHECK(!memcmp(original,smc.fx_strip.map,sizeof original));
    CHECK(smc_page_now()==page);
    CHECK(smc_ui_active()); /* consuming an ambiguous edit keeps the held-effect screen */
    arrows=(arw_t){0}; CHECK(arw_frame(&arrows,1,ARW_SELECT,now_ms)==0);
    CHECK(!memcmp(original,smc.fx_strip.map,sizeof original));
    message(0x80,8,0); message(0x80,0,0); CHECK(!smc.fx_down);

    /* All main SELECT keys play immediately; a fast mapping chord cannot navigate on release. */
    for(int note=29;note<=31;note++) {
        setup(V_FX,PART_303B); message(0x90,note,127);
        CHECK(smc.fx_down==(1u<<(note-24)));
        tap(0x63); message(0x80,note,0);
        CHECK(smc.view==V_FX && !smc.fx_down && !smc.fx_main_wait);
    }
    setup(V_FX,PART_303B); message(0x90,29,127); tap(0x63);
    smc.select_down=0; smc_poll(); /* lost key-up must still clear after tap eligibility was revoked */
    CHECK(!smc.fx_down && !smc.fx_owned && smc.fx_strip.key_bus[29]==SMCFX_EMPTY);
    message(0x80,29,0); CHECK(smc.view==V_FX);
    setup(V_FX,PART_303B); smc_fx_load_page(1); message(0x90,0,127);
    smc.view=V_HOME; smc_poll(); CHECK(smc.fx_strip.key_bus[0]==SMCFX_EMPTY);
    smc.view=V_FX; CHECK(!smc_fx_keyboard_arrow(1)); /* old blank key cannot acquire a new context */
    setup(V_FX,PART_303B); message(0x90,6,127); tap(0x63); message(0x80,6,0);
    CHECK(smc.view==V_FX && smc.fx_page==0 && !smc.fx_down); /* physical HOME FX owns this chord */
}

/* Catches an arrow interceptor stealing modal, clip or unrelated keyboard input. */
static void held_fx_arrow_guards(void)
{
    setup(V_FX,PART_303B); message(0x90,0,127);
    ui.help=1; CHECK(!smc_fx_keyboard_arrow(1)); ui.help=0;
    ui.overlay=O_LIST; CHECK(!smc_fx_keyboard_arrow(1)); ui.overlay=O_ASK;
    CHECK(!smc_fx_keyboard_arrow(1)); ui.overlay=O_NONE;
    clip_grid_mode=1; CHECK(!smc_fx_keyboard_arrow(1)); clip_grid_mode=0;
    project_busy=1; CHECK(!smc_fx_keyboard_arrow(1)); project_busy=0;
    CHECK(smc.fx_strip.map[0][0][0]==SMCFX_REV);
    message(0x80,0,0); CHECK(!smc_fx_keyboard_arrow(1));
    setup(V_HOME,PART_303B); message(0x90,0,127);
    CHECK(!smc_fx_keyboard_arrow(1));
}

/* Catches requiring a central BLE link for an otherwise valid USB/User hold. */
static void held_fx_host_arrows(void)
{
    setup(V_FX,PART_303B); smc_ble_link(0); smc_poll();
    proj.set.bt|=PROJ_BT_SMC_USER; smc_poll();
    CHECK(!smc.up);
    CHECK(smc_midi_host(0x09u | 0x9fu<<8 | 127u<<24)); smc_poll();
    CHECK(smc.fx_down==1u);
    CHECK(smc_midi_host(0x09u | 0x9fu<<8 | 0x63u<<16 | 127u<<24)); smc_poll();
    CHECK(smc.fx_strip.map[0][0][0]==SMCFX_DLY);
    arw_t arrows={0}; CHECK(arw_frame(&arrows,1,ARW_SELECT,now_ms)==0);
    CHECK(smc.fx_strip.map[0][0][0]==SMCFX_REV);
    CHECK(smc_midi_host(0x08u | 0x8fu<<8)); smc_poll(); CHECK(!smc.fx_down);
}

#if SMC_PEERS > 1
/* Several held FX keys consume both arrow routes, including across peers. */
static void held_fx_peer_arrows(void)
{
    setup(V_FX,PART_303B); smc_ble_link_slot(1,1); smc_poll();
    smc_slot=1; smc.view=V_FX; smc.part=smc.mixsel=PART_303B; smc_fx_load_page(0);
    smc_slot=0; message(0x90,0,127);
    uint8_t down[]={0x90,0,127}, up[]={0x80,0,0};
    smc_ble_midi_slot(1,down,3); smc_poll();
    arw_t arrows={0}; CHECK(arw_frame(&arrows,2,ARW_SELECT,now_ms)==0);
    CHECK(smc_states[0].fx_strip.map[0][0][0]==SMCFX_REV);
    CHECK(smc_states[1].fx_strip.map[0][0][0]==SMCFX_REV && smc_slot==0);
    tap(0x63); CHECK(smc_states[0].fx_strip.map[0][0][0]==SMCFX_REV);
    CHECK(smc_states[1].fx_strip.map[0][0][0]==SMCFX_REV);
    CHECK(smc_ui_active());
    message(0x80,0,0);
    tap(0x63); /* A controller cannot edit another peer's sole held key. */
    CHECK(smc_states[0].fx_strip.map[0][0][0]==SMCFX_REV);
    CHECK(smc_states[1].fx_strip.map[0][0][0]==SMCFX_REV && smc_slot==0);
    CHECK(arw_frame(&arrows,0,ARW_SELECT,now_ms)==0);
    CHECK(arw_frame(&arrows,2,ARW_SELECT,now_ms)==0);
    CHECK(smc_states[1].fx_strip.map[0][0][0]==SMCFX_DLY && smc_slot==0);
    smc_ble_midi_slot(1,up,3); smc_poll();
    CHECK(!smc_states[0].fx_down && !smc_states[1].fx_down);
    message(0x90,0,127); tap(0x63);
    CHECK(smc_states[0].fx_strip.map[0][0][0]==SMCFX_DLY);
    CHECK(smc_states[1].fx_strip.map[0][0][0]==SMCFX_DLY); /* earlier peer-one edit retained */
    message(0x80,0,0);
}
#endif

/* Catches a missing/wrong topbar effect glyph or a stale glyph after key-up. */
static void held_fx_header(void)
{
    setup(V_FX,PART_303B); message(0x90,0,127); smc_ui_draw_peer();
    memcpy(before_fb,fb,sizeof fb);
    cv_begin(240,HDR_H,C_BLACK); ui_pictogram(182,1,UI_G_FX_REVERB,2,C_HI); cv_commit(0,0,0);
    for(int y=1;y<19;y++) CHECK(!memcmp(before_fb+y*240+182,fb+y*240+182,18*sizeof *fb));
    tap(0x63); smc_ui_draw_peer();
    CHECK(changed_rows(0,HDR_H,before_fb,fb)>0); memcpy(before_fb,fb,sizeof fb);
    cv_begin(240,HDR_H,C_BLACK); ui_pictogram(182,1,UI_G_FX_DELAY,2,C_HI); cv_commit(0,0,0);
    for(int y=1;y<19;y++) CHECK(!memcmp(before_fb+y*240+182,fb+y*240+182,18*sizeof *fb));
    message(0x80,0,0); smc_ui_draw_peer();
    CHECK(changed_rows(0,HDR_H,before_fb,fb)>0);
    message(0x90,0,127); message(0x90,1,127); message(0x80,1,0);
    CHECK(smc_ui.action==0 && smc_ui.strip==0); /* remaining glyph, KEY number and footer share a target */
    CHECK(smc.fx_down==1u);
    tap(0x63); CHECK(smc.fx_strip.map[0][0][0]==SMCFX_STUT);
    message(0x80,0,0); CHECK(smc_ui.action==0 && !smc.fx_down);
}

static void footer_toggle_is(int on)
{
    smc_ui_draw_peer();
    CHECK((fb[(KNOB_Y+32)*240+129]!=0)==!on);
    CHECK((fb[(KNOB_Y+32)*240+140]!=0)==!!on);
}
static void pictured_action_states(void)
{
    setup(V_HOME,PART_303B);
    tap(0); CHECK(ui.rec); footer_toggle_is(1);
    tap(0); CHECK(!ui.rec); footer_toggle_is(0);
    tap(16); CHECK(part_muted(PART_909)); footer_toggle_is(1);
    tap(16); CHECK(!part_muted(PART_909)); footer_toggle_is(0);
    message(0x90,8,127); CHECK(smc.solo); footer_toggle_is(1);
    message(0x80,8,0); CHECK(!smc.solo); footer_toggle_is(0);
    tap(0x5e); render_audio(1); CHECK(seq.playing && !smc_ui_active());
    tap(0x5e); render_audio(1); CHECK(!seq.playing && !smc_ui_active());
    setup(V_FX,PART_303B); message(0x90,24,127); smc_ui_draw_peer();
    CHECK(smc.fx_down&1u); memcpy(before_fb,fb,sizeof fb);
    message(0x80,24,0); smc_ui_draw_peer(); CHECK(!(smc.fx_down&1u));
    CHECK(changed_rows(KNOB_Y+49,KNOB_Y+58,before_fb,fb)>0);
    setup(V_MIX,PART_909); smc_page_set(1); tap(0x2f);
    CHECK(smc_button_symbol(23)==UI_G_NONE && !strcmp(smc_button_caption(23),"--"));
}
static void composed_header_cues(void)
{
    setup(V_SEQ,PART_303B);
    seq.bpm=120; seq.ext=0; seq.ppat[PART_303B]=63;
    proj.pat[63].bass[PART_303B-PART_303A].len=64;
    ui.spage=3; seq.chain_a=seq.chain_b=0; seq.pcue[PART_303B]=0xff;
    ui.msg_until=0; ui.frame=16; draw_header(); memcpy(before_fb,fb,sizeof fb);
    seq.chain_b=1; draw_header();
    for(int y=0;y<HDR_H;y++)
        CHECK(!memcmp(before_fb+y*240+191,fb+y*240+191,22*sizeof *fb));
    seq.pcue[PART_303B]=62; draw_header(); memcpy(active_fb,fb,sizeof fb);
    ui.spage=0; seq.chain_b=0; draw_header();
    CHECK(!memcmp(active_fb,fb,HDR_H*240*sizeof *fb)); /* Pending cue owns annotation space. */
    for(int y=0;y<HDR_H;y++)
        CHECK(!memcmp(before_fb+y*240+191,fb+y*240+191,22*sizeof *fb));
}
/* Catches oversized cell glyphs, colour-only state and cue/tempo collisions. */
static void pictogram_rendering(void)
{
    setup(V_HOME,PART_303B);
    for(int g=UI_G_NONE+1;g<UI_G_COUNT;g++) {
        cv_begin(240,HDR_H,C_BLACK); ui_pictogram(20,4,g,1,C_WHITE); cv_commit(0,0,0);
        unsigned ink=0;
        for(int y=0;y<HDR_H;y++) for(int x=0;x<240;x++) {
            if(x>=20 && x<29 && y>=4 && y<13) ink+=fb[y*240+x]!=0;
            else CHECK(!fb[y*240+x]);
        }
        CHECK(ink>0);
    }
    cv_begin(240,HDR_H,C_BLACK); ui_state_toggle(30,4,0,C_WHITE); cv_commit(0,0,0);
    CHECK(fb[8*240+33]!=0 && fb[8*240+44]==0);
    memcpy(before_fb,fb,sizeof fb);
    cv_begin(240,HDR_H,C_BLACK); ui_state_toggle(30,4,1,C_WHITE); cv_commit(0,0,0);
    CHECK(fb[8*240+33]==0 && fb[8*240+44]!=0);
    CHECK(memcmp(before_fb,fb,sizeof fb));
    clip_grid_mode=1;
    CHECK(smc_button_symbol(7)==UI_G_NEXT && smc_button_symbol(15)==UI_G_PREV);
    CHECK(smc_button_symbol(31)==UI_G_NEXT);
    clip_grid_mode=0;
    CHECK(smc_button_symbol(0)==UI_G_RECORD && smc_button_symbol(8)==UI_G_SOLO);
    CHECK(smc_button_symbol(16)==UI_G_MUTE && smc_button_symbol(31)==UI_G_MIX);
    smc_page_set(0);
    for(int p=1;p<=NPAT;p++) {
        cv_begin(240,HDR_H,C_BLACK);
        int end=ui_position_badge(UI_G_PATTERN,p,NPAT,C_WHITE,C_GRAY);
        cv_commit(0,0,0); CHECK(end<=185);
        for(int y=0;y<HDR_H;y++) for(int x=185;x<240;x++) CHECK(!fb[y*240+x]);
    }
    cv_begin(240,HDR_H,C_BLACK);
    CHECK(ui_position_badge(UI_G_SONG,192,0,C_AMB,C_GRAY)<=185);
    cv_commit(0,0,0);
    screen_state_t keyboard=screen_state();
    fader(0,0); smc_ui_draw_peer(); keyboard_unchanged(keyboard);
    CHECK(smc_same(smc_target(0),PR(R_ENG,T_MIX,PART_909,MX_LEVEL)));
}
#endif

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
    setup(V_HOME,PART_303B); seq.ppat[PART_909]=63; seq.pcue[PART_909]=62;
    ui.msg_until=0; ui.frame=16; draw_header(); shot("glyph-pattern-cue");
    ui.frame=0; draw_header(); shot("glyph-pattern-cue-dim");
    seq.song_on=1; seq.song_pos=191; seq.playing=1;
    draw_header(); shot("glyph-song-position");
    setup(V_SEQ,PART_303B); seq.bpm=120; seq.ext=0; seq.ppat[PART_303B]=63;
    proj.pat[63].bass[PART_303B-PART_303A].len=64;
    ui.spage=3; seq.chain_a=seq.chain_b=0; seq.pcue[PART_303B]=0xff;
    ui.msg_until=0; draw_header(); shot("glyph-pattern-range");
    seq.chain_b=1; draw_header(); shot("glyph-pattern-chain");
    setup(V_HOME,PART_303B); pref_set(smc_target(0),100); fader(0,0);
    smc_ui_draw_peer(); shot("glyph-main-mixer"); shot("glyph-pickup-up");
    fader(0,127); smc_ui_draw_peer(); shot("glyph-acquired");
    smc.picked=0; pref_set(smc_target(0),0); smc_ui_begin(0,0);
    smc_ui_draw_peer(); shot("glyph-pickup-down");
    tap(0x5d); smc_ui_draw_peer(); shot("glyph-locked-mixer");
    tap(0x61); smc_ui_draw_peer(); shot("glyph-knob-bank");
    setup(V_HOME,PART_303B); tap(0); smc_ui_draw_peer(); shot("glyph-record-on");
    tap(0); smc_ui_draw_peer(); shot("glyph-record-off");
    tap(16); smc_ui_draw_peer(); shot("glyph-mute-on");
    tap(16); smc_ui_draw_peer(); shot("glyph-mute-off");
    message(0x90,8,127); smc_ui_draw_peer(); shot("glyph-solo-held");
    message(0x80,8,0); smc_ui_draw_peer(); shot("glyph-solo-release");
    tap(0x5e); render_audio(1); ui_frame(); shot("glyph-play");
    tap(0x5e); render_audio(1); ui_frame(); shot("glyph-stop");
    setup(V_FX,PART_303B); message(0x90,24,127);
    smc_ui_draw_peer(); shot("glyph-fx-held");
    message(0x80,24,0); smc_ui_draw_peer(); shot("glyph-fx-idle");
    /* Same production input sequence on af1a3a5e and this candidate. */
    setup(V_FX,PART_303B); message(0x90,0,127);
    smc_ui_draw_peer(); shot("fx-arrow-held");
    tap(0x63); smc_ui_draw_peer(); shot("fx-arrow-right");
    tap(0x62); smc_ui_draw_peer(); shot("fx-arrow-left");
    message(0x80,0,0); smc_ui_draw_peer(); shot("fx-arrow-release");
    message(0x90,0,127); message(0x90,1,127); message(0x80,1,0);
    smc_ui_draw_peer(); shot("fx-arrow-focus-return"); message(0x80,0,0);
    setup(V_FX,PART_303B); smc_fx_load_page(1); message(0x90,0,127);
    smc_ui_draw_peer(); shot("fx-arrow-empty");
    tap(0x62); smc_ui_draw_peer(); shot("fx-arrow-empty-left");
    message(0x80,0,0);
    setup(V_FX,PART_303B); message(0x90,29,127);
    smc_ui_draw_peer(); shot("fx-arrow-main-quick");
    tap(0x63); smc_ui_draw_peer(); shot("fx-arrow-main-edit");
    message(0x80,29,0); smc_ui_draw_peer(); shot("fx-arrow-main-release");
    /* Exercise each real effect symbol without needing new BEFORE helpers. */
    for(int fx=0;fx<SMCFX_NFX;fx++) {
        int note=fx==SMCFX_GATE?2:fx==SMCFX_HP || fx==SMCFX_LP ||
            fx==SMCFX_GATE8 || fx==SMCFX_GATE16 || fx==SMCFX_TRANCE?4:0;
        setup(V_FX,PART_303B); smc_fx_load_page(0);
        smc.fx_strip.map[0][smc_fx_bus((uint8_t)note)][0]=(uint8_t)fx;
        message(0x90,note,127); smc_ui_draw_peer();
        char scene[32]; snprintf(scene,sizeof scene,"fx-effect-%d",fx); shot(scene);
        message(0x80,note,0);
    }
    setup(V_MIX,PART_909); smc_page_set(1); tap(0x2f); fader(0,0);
    smc_ui_draw_peer(); shot("glyph-drum-bank2");
}

#ifndef F4_CAPTURE_DRIVER
int main(void)
{
    CHECK(X0X_ORIGINAL_SYNTH==1 && PROJ_SND_REV==13);
    strip8_modes(); clip_stage_and_banks(); clip_stage_drum_contexts(); locked_navigation_and_fx();
    clock_icons(); retained_contracts(); pictogram_rendering();
    pictured_action_states(); composed_header_cues();
    held_fx_arrows(); held_fx_arrow_guards(); held_fx_host_arrows(); held_fx_header();
#if SMC_PEERS > 1
    held_fx_peer_arrows();
#endif
    printf("smc-remainder: %u checks PASS\n",checks);
    return 0;
}
#endif
