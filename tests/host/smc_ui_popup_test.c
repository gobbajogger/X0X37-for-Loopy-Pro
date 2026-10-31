/* SPDX-License-Identifier: GPL-3.0-only */
/* Real SMC queue/dispatcher/renderer: no keyboard navigation, even temporarily. */
#define SMC_UI_MAIN legacy_ui_regressions
#include "smc_ui_test.c"
#undef SMC_UI_MAIN

static void keyboard_unchanged(screen_state_t before)
{
    screen_state_t after = screen_state();
    CHECK(before.view == after.view && before.part == after.part);
    CHECK(before.mixsel == after.mixsel && before.prev_view == after.prev_view);
    CHECK(!memcmp(before.page, after.page, sizeof before.page));
    CHECK(!memcmp(before.sel, after.sel, sizeof before.sel));
    CHECK(before.overlay == after.overlay && before.help == after.help);
    CHECK(before.list_sel == after.list_sel && before.list_top == after.list_top);
    CHECK(before.clip == after.clip && before.clip_page == after.clip_page);
}

static void test_locked_bank_recapture(void)
{
    setup(V_MIX,PART_909);smc_page_set(1);
    smc.sel[PART_909]=0;
    CHECK(smc_lock_toggle());
    smc_select_drum(PART_909,8); /* Navigation may move beyond the captured bank. */
    screen_state_t keyboard=screen_state();
    smc_knob_bank(1);
    CHECK(smc.lock_bank==0 && smc.sel[PART_909]==8);
    CHECK(smc_target(0).a==T_909 && smc_target(0).b==0);
    CHECK(smc_knob_target(0).a==T_909 && smc_knob_target(0).b==0);
    keyboard_unchanged(keyboard);
    smc_page_set(2);smc.sel[PART_808]=8;
    smc_knob_bank(1);
    CHECK(smc.sel[PART_808]==8);
    CHECK(smc_target(0).a==T_808 && smc_target(0).b==0);
    CHECK(smc_knob_target(0).a==T_808 && smc_knob_target(0).b==0);
    keyboard_unchanged(keyboard);
}

static void test_help_musical_buttons(void)
{
    setup(V_HOME,PART_303A);ui.help=1;
    screen_state_t keyboard=screen_state();
    int muted=part_muted(PART_909),rec=ui.rec;
    message(0x90,16,127);CHECK(part_muted(PART_909)!=muted);
    message(0x80,16,0);keyboard_unchanged(keyboard);
    message(0x90,8,127);CHECK(smc.solo & 1u);keyboard_unchanged(keyboard);
    message(0x80,8,0);CHECK(!(smc.solo & 1u));keyboard_unchanged(keyboard);
    message(0x90,0x5f,127);CHECK(ui.rec!=rec);keyboard_unchanged(keyboard);
    message(0x80,0x5f,0);
    message(0x90,0x5e,127);CHECK(seq.req_start);keyboard_unchanged(keyboard);
    message(0x80,0x5e,0);keyboard_unchanged(keyboard);
    CHECK(ui.help==1);
}

static void test_locked_bass_identity(void)
{
    setup(V_PART,PART_303A);
    CHECK(smc_lock_toggle());
    pref_t target=smc_knob_target(0);
    CHECK(target.kind==R_ENG && target.a==T_303 && target.b==0);
    int bank=engine_bass_bank(0);
    engine_bass_off(0);engine_bass_off(1);
    CHECK(engine_bass_select_preset(0,bank==ORIGINAL_SOUND_CHICAGO?
        ORIGINAL_SOUND_MIAMI:ORIGINAL_SOUND_CHICAGO,0,&proj.sound)==0);
    for(int i=0;i<1024 && engine_bass_preset_pending(0);i++) {
        int32_t audio[512];engine_render(audio,256);
    }
    CHECK(!engine_bass_preset_pending(0) && engine_bass_bank(0)!=bank);
    CHECK(smc_knob_target(0).kind==R_NONE);
    knob(0,1); /* The rejected reference must release its engine edit hold. */
    CHECK(smc.locked);
    smc_knob_bank(1);
    CHECK(smc_knob_target(0).kind==R_ENG);
    CHECK(smc.lock_bass_bank[0]==engine_bass_bank(0));
    CHECK(engine_bass_select_preset(0,bank,0,&proj.sound)==0);
    for(int i=0;i<1024 && engine_bass_preset_pending(0);i++) {
        int32_t audio[512];engine_render(audio,256);
    }
    CHECK(!engine_bass_preset_pending(0) && engine_bass_bank(0)==bank);
}

/* DAW notes, User MMC and host routes all use the real production queues. */
static void transport_event(int route, int note, int down)
{
    if (route == 1 || route >= 3) {
        if (!down) return; /* MMC has no physical release. */
        int command = 33;
        while (command <= 43 && smc_mmc_note(command) != note) command++;
        CHECK(command <= 43);
        if (route >= 3) smc_mmc_host((uint8_t)command, route == 4);
        else {
            uint8_t m[6] = {0xf0,0x7f,0x7f,6,(uint8_t)command,0xf7};
            smc_ble_midi(m,sizeof m);
        }
    } else if (route == 2) {
        CHECK(smc_midi_host((down ? 0x09u : 0x08u) |
            (down ? 0x9fu : 0x8fu) << 8 | (uint32_t)note << 16 | (down ? 127u << 24 : 0)));
    } else {
        message(down ? 0x90 : 0x80,note,down ? 127 : 0);
        return;
    }
    smc_poll();
}

static void transport_setup(int route, int locked)
{
    setup(V_SEQ,PART_909);
    if (route) proj.set.bt |= PROJ_BT_SMC_USER;
    seq.ppat[PART_909]=3;seq.pcue[PART_909]=0xff;
    if (locked) { CHECK(smc_lock_toggle());smc_select_part(PART_808); }
    smc_ui_cancel();
    ui.part=PART_303B;set_view(V_FX);*page_of(V_FX)=1;build_pages();
}

static void test_transport_without_popup(void)
{
    for(int route=0;route<5;route++) {
        transport_setup(route,0);screen_state_t keyboard=screen_state();
        transport_event(route,0x5e,1);
        CHECK(seq.req_start && !seq.req_stop);CHECK(!smc_ui_active());
        transport_event(route,0x5e,0);keyboard_unchanged(keyboard);
        transport_event(route,0x5e,1);
        CHECK(seq.req_stop && !seq.req_start);CHECK(!smc_ui_active());
        transport_event(route,0x5e,0);keyboard_unchanged(keyboard);
        if(route==1 || route>=3) {
            seq_start(&seq);transport_event(route,0x5d,1);
            CHECK(seq.req_stop && !seq.req_start);CHECK(!smc.locked && !smc_ui_active());
            keyboard_unchanged(keyboard);
        }
    }
}

static void pattern_notice(const char *value)
{
    char expected[24];put_s(put_s(expected,"NEXT: PAT "),value);
    CHECK(!strcmp(ui.msg,expected));
    ui_frame();memcpy(active_fb,fb,sizeof fb);
    cv_begin(240,HDR_H,C_BLACK);
    night_panel(0,0,240,HDR_H,0);
    box(0,0,240,HDR_H-1,part_col());
    char arrow[16];put_s(put_s(arrow,"> "),value);
    int x=(240-tw(&FONT_B,"NEXT")-6-12-6-tw(&FONT_B,arrow))/2;
    x=cv_text(x,vc(&FONT_B,0,HDR_H-1),&FONT_B,"NEXT",C_BLACK)+6;
    frame(x,1,12,17,C_BLACK);
    /* Hand-derived from the stored fixtures below, independently of HOME's
     * projection and glyph renderer. Each instrument occupies a distinct row. */
    if(!strcmp(value,"5")) {
        cv_rect(x+2,2,1,3,C_BLACK);cv_rect(x+9,2,1,3,C_BLACK); /* 909: steps 0,63 */
        cv_rect(x+4,5,1,3,C_BLACK); /* 808: step 16 of 64 */
        cv_rect(x+7,8,1,3,C_BLACK); /* BREAK: step 10 */
        cv_rect(x+5,11,1,3,C_BLACK); /* 303 B: step 6 */
        cv_rect(x+9,14,1,3,C_BLACK); /* 303 A: step 14 */
    } else if(!strcmp(value,"6")) {
        cv_rect(x+4,8,1,3,C_BLACK); /* BREAK: step 4 */
    }
    cv_text(x+18,vc(&FONT_B,0,HDR_H-1),&FONT_B,arrow,C_BLACK);
    cv_commit(0,0,0);
    CHECK(!memcmp(active_fb,fb,HDR_H*240*sizeof *fb));
    CHECK(ui.msg_until==now_ms+1000);
}

static void test_pattern_notice(void)
{
    for(int route=0;route<5;route++) for(int locked=0;locked<2;locked++)
    for(int open=0;open<2;open++) {
        transport_setup(route,locked);screen_state_t keyboard=screen_state();
        memset(proj.arr.lane,0,sizeof proj.arr.lane);
        for(int pat=4;pat<=5;pat++) {
            memset(&proj.pat[pat],0,sizeof proj.pat[pat]);
            for(int p=0;p<NKIT;p++) proj.pat[pat].drum[p].len=64;
            for(int p=0;p<NBASS;p++) proj.pat[pat].bass[p].len=16;
        }
        sm_set(proj.pat[4].drum[0].hit[0],0);
        sm_set(proj.pat[4].drum[0].hit[0],63);
        sm_set(proj.pat[4].drum[1].hit[0],16);
        proj.pat[4].brk.steps=1u<<10;
        proj.pat[4].bass[1].step[6].flags=G_NOTE;
        proj.pat[4].bass[1].step[6].velocity=120;
        proj.pat[4].bass[0].step[14].flags=G_NOTE;
        proj.pat[4].bass[0].step[14].velocity=100;
        proj.pat[5].brk.steps=1u<<4;
        if(open) smc_fader(0,0); /* Establish the same existing popup for every input route. */
        CHECK(smc_ui_active()==open);
        ui_frame();memcpy(before_fb,fb,sizeof fb);
        uint8_t popup[sizeof smc_ui];memcpy(popup,&smc_ui,sizeof popup);
        unsigned forces=clip_forces;
        transport_event(route,0x5c,1);
        CHECK(seq_cue_of(&seq,PART_909)==4 && seq_cue_of(&seq,PART_808)==-1);
        CHECK(smc_ui_active()==open);CHECK(!memcmp(popup,&smc_ui,sizeof popup));
        CHECK(clip_forces==forces);pattern_notice("5");keyboard_unchanged(keyboard);
        CHECK(!memcmp(before_fb+HDR_H*240,active_fb+HDR_H*240,(240-HDR_H)*240*sizeof *fb));
        transport_event(route,0x5c,0);
        transport_event(route,0x5c,1);CHECK(seq_cue_of(&seq,PART_909)==5);
        pattern_notice("6");transport_event(route,0x5c,0);
        transport_event(route,0x5b,1);CHECK(seq_cue_of(&seq,PART_909)==4);
        pattern_notice("5");transport_event(route,0x5b,0);
        transport_event(route,0x5b,1);CHECK(seq_cue_of(&seq,PART_909)==-1);
        pattern_notice("--");transport_event(route,0x5b,0);
        seq.ppat[PART_909]=0;transport_event(route,0x5b,1);
        CHECK(seq_cue_of(&seq,PART_909)==-1);pattern_notice("--");
        transport_event(route,0x5b,0);keyboard_unchanged(keyboard);
        CHECK(!seq.playing && !seq.req_start && !seq.req_stop);
        /* Keep an existing popup alive while its notification alone expires. */
        now_ms+=999;
        if(open) smc_ui.until=now_ms+SMC_UI_MS;
        ui_frame();CHECK(!memcmp(active_fb,fb,HDR_H*240*sizeof *fb));
        now_ms++;
        ui_frame();CHECK(smc_ui_active()==open);
        CHECK(memcmp(active_fb,fb,HDR_H*240*sizeof *fb)); /* expired toast is gone */
        keyboard_unchanged(keyboard);
    }
}

int main(void)
{
    test_transport_without_popup();
    test_pattern_notice();
    test_locked_bank_recapture();
    test_help_musical_buttons();
    test_locked_bass_identity();
    static const int views[] = {V_HOME,V_PART,V_GEN,V_FX,V_MIX,V_GLO,V_SONG,V_SEQ};
    static const uint8_t nav[] = {5,6,7,13,14,15,24,25,26,27,28,29,30,31,
                                  0x2e,0x2f,0x30,0x31,0x60,0x61,0x62,0x63};
    for(unsigned v=0;v<sizeof views/sizeof *views;v++) {
        setup(views[v],PART_303A);
        ui.mixsel=PART_808;ui.sel[0]=10;ui.sel[1]=9;
        *page_of(V_FX)=1;ui_frame();
        screen_state_t saved=screen_state();
        pref_t target=smc_target(0);
        for(int n=0;n<8;n++) {
            fader(n,0);keyboard_unchanged(saved);
            knob(n,1);keyboard_unchanged(saved);
        }
        for(unsigned n=0;n<sizeof nav;n++) {
            message(0x90,nav[n],127);keyboard_unchanged(saved);
            message(0x80,nav[n],0);keyboard_unchanged(saved);
        }
        smc_lock_toggle();keyboard_unchanged(saved);
        message(0x90,0x63,127);message(0x80,0x63,0);keyboard_unchanged(saved);
        smc_lock_toggle();keyboard_unchanged(saved);
        (void)target;
        now_ms+=1300;ui_frame();keyboard_unchanged(saved);CHECK(!smc_ui_active());
    }
    setup(V_SEQ,PART_303A);shot("before-keyboard");
    screen_state_t saved=screen_state();
    fader(0,0);ui_frame();keyboard_unchanged(saved);CHECK(smc_ui_active());shot("after-fader-popup");
    knob(0,1);ui_frame();keyboard_unchanged(saved);shot("after-knob-popup");
    message(0x90,16,127);ui_frame();keyboard_unchanged(saved);shot("after-mute-popup");
    now_ms+=1300;CHECK(smc_ui_active()); /* held buttons retain the overlay */
    message(0x80,16,0);now_ms+=1300;ui_frame();CHECK(!smc_ui_active());
    keyboard_unchanged(saved);shot("after-dismiss-keyboard");
    /* A panel page change must survive popup expiry rather than be restored. */
    fader(0,1);set_view(V_FX);*page_of(V_FX)=1;build_pages();
    saved=screen_state();now_ms+=1300;ui_frame();keyboard_unchanged(saved);
    /* Modal answers remain explicit keyboard-owned actions; ordinary SMC input
     * leaves the dialog and its remembered navigation alone. */
    ask(ACT_SONG_CLR,0,"TEST",0);saved=screen_state();
    fader(0,2);knob(0,1);keyboard_unchanged(saved);
    message(0x90,6,127);message(0x80,6,0);CHECK(ui.overlay==O_NONE);
    ask(ACT_SONG_CLR,0,"TEST",0);
    message(0x90,5,127);message(0x80,5,0);CHECK(ui.overlay==O_NONE);
    printf("smc_own_popup: PASS (%u checks, immediate keyboard invariance)\n",checks);
    return 0;
}
