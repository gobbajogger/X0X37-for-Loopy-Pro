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

int main(void)
{
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
    /* The controller does not enter, confirm or cancel a keyboard modal. */
    ask(ACT_SONG_CLR,0,"TEST",0);saved=screen_state();
    message(0x90,5,127);message(0x80,5,0);keyboard_unchanged(saved);
    message(0x90,6,127);message(0x80,6,0);keyboard_unchanged(saved);
    printf("smc_own_popup: PASS (%u checks, immediate keyboard invariance)\n",checks);
    return 0;
}
