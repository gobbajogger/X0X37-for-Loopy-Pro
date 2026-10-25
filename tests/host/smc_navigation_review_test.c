/* SPDX-License-Identifier: GPL-3.0-only */
/* Independent boundary probes using production dispatch and UI. */
#define SMC_UI_MAIN smc_original_ui_main
#include "smc_ui_test.c"
static void tap_review(int note) { message(0x90,note,127); message(0x80,note,0); }
static void test_bound_acceleration(void)
{
    setup(V_GEN,PART_303A); seq.ppat[PART_303A]=0; build_pages();
    accel_off=0;
    for(int i=0;i<5;i++) knob(0,1);
    CHECK(smc.knob_run>=4); CHECK(smc.knob_target.b==1);
    seq.ppat[PART_303A]=1;
    pref_t next=smc_bind(smc_knob_target(0)); pref_set(next,20);
    knob(0,1); CHECK(pref_get(next)==21); CHECK(smc.knob_run==1); CHECK(smc.knob_target.b==2);
    setup(V_SONG,PART_303A); song_fill(6); ui.song_sel=0; build_pages();
    knob(1,1); CHECK(smc.knob_target.b==1);
    smc.knob_run=100; smc.knob_t=now_ms; ui.song_sel=1;
    knob(1,1); CHECK(smc.knob_target.b==2); CHECK(smc.knob_run<=1);
    setup(V_GEN,PART_303A); CHECK(smc_lock_toggle()); knob(0,1);
    pref_t captured=smc.knob_target; seq.ppat[PART_303A]=3; knob(0,1);
    CHECK(smc_same(captured,smc.knob_target));
    puts("PASS acceleration identity follows actual pattern/song destination and retains locked binding");
}
static void test_locked_captions(void)
{
    const int notes[]={5,6,14,15,29,30,31};
    const char *captions[7];
    for(int global=0;global<2;global++) {
        setup(V_HOME,PART_303A); if(global) tap_review(14);
        CHECK(smc_lock_toggle());
        for(unsigned i=0;i<7;i++) captions[i]=smc_button_caption(notes[i]);
        ui.part=PART_303B; ui.view=global ? V_HOME : V_GLO;
        ui.overlay=global ? O_NONE : O_LIST; build_pages(); clip_grid_mode=1; ck.pg=1;
        for(unsigned i=0;i<7;i++) CHECK(!strcmp(captions[i],smc_button_caption(notes[i])));
    }
    puts("PASS locked action captions survive keyboard menu/view/clip changes");
    setup(V_HOME,PART_303A); tap_review(0x61); CHECK(smc.bank==1); CHECK(smc_lock_toggle());
    pref_t locked=smc_knob_target(0); const char *bank=smc_bank_name();
    ask(ACT_RESET,0,"RESET PROJECT?","SAMPLES ARE KEPT");
    CHECK(ui.overlay==O_ASK); tap_review(6); CHECK(ui.overlay==O_NONE);
    CHECK(smc.locked && smc.bank==1); CHECK(smc_same(locked,smc_knob_target(0)));
    CHECK(!strcmp(bank,smc_bank_name()));
    puts("PASS dismissing confirmation retains nonzero locked bank, parameter refs and bank caption");
}
int main(void)
{
    int failures=0;
    test_bound_acceleration(); test_locked_captions();
    setup(V_SEQ, PART_909);
    CHECK(smc_lock_toggle());
    clip_grid_mode=1; ck.pg=1;
    tap_review(0x61);
    printf("locked drum Up/Down after keyboard clip: bank=%u clip_page=%u lock_view=%u\n",smc.bank,ck.pg,smc.lock_view);
    if (smc.bank != 1 || ck.pg != 1) { puts("FAIL keyboard clip redirects locked SMC bank button"); failures++; }
    setup(V_PART, PART_303A);
    tap_review(5); CHECK(ui.overlay==O_LIST);
    uint8_t old_sel=ui.list_sel;
    CHECK(smc_lock_toggle());
    tap_review(0x61);
    printf("locked list Down: overlay=%u selection=%u old=%u bank=%u\n",ui.overlay,ui.list_sel,old_sel,smc.bank);
    if (ui.overlay != O_LIST || ui.list_sel == old_sel) { puts("FAIL locked list loses menu navigation ownership"); failures++; }
    setup(V_HOME, PART_303A);
    tap_review(14); CHECK(ui.view==V_GLO && ui.overlay==O_LIST);
    pref_t global_first=list_rows[1]; int global_count=ui.list_n;
    CHECK(smc_lock_toggle());
    tap_review(0x61);
    printf("locked global Down: list_count=%u was=%d first_kind=%u was=%u\n",ui.list_n,global_count,list_rows[1].kind,global_first.kind);
    if (ui.list_n != global_count || !smc_same(global_first,list_rows[1])) { puts("FAIL locked global list rebuilt as ordinary parameter pages"); failures++; }
    setup(V_HOME, PART_303A); tap_review(0x61); CHECK(smc_lock_toggle());
    const char *bank_name=smc_bank_name();
    ui.overlay=O_LIST;
    printf("locked main bank label after keyboard menu: before='%s' after='%s'\n",bank_name,smc_bank_name());
    if (strcmp(bank_name,smc_bank_name())) { puts("FAIL locked bank caption follows keyboard overlay"); failures++; }
    setup(V_HOME,PART_303A); tap_review(15); message(0x90,15,127);
    CHECK(cur_page()==1); message(0x90,15,127); CHECK(cur_page()==1);
    uint8_t noop[3]={0xB0,16,0}, release[3]={0x80,15,0};
    for (unsigned i=0;i<SMC_Q;i++) smc_ble_midi(noop,3);
    smc_ble_midi(release,3); smc_poll();
    message(0x90,15,127); CHECK(cur_page()==2); message(0x80,15,0);
    puts("PASS held shortcut repeat suppressed; overflow-lost keyup rearms next physical press");
    smc_project_reset(); message(0x90,15,127); CHECK(cur_page()==0);
    smc_ble_link(0); smc_poll(); smc_ble_link(1); smc_poll();
    message(0x90,15,127); CHECK(cur_page()==1); message(0x80,15,0);
    puts("PASS project reset and reconnect clear shortcut press edges");
    setup(V_HOME,PART_303A); pref_t level=smc_target(0);
    pref_set(level,70); smc_frame(); fader(0,smc_scaled(level)); CHECK(smc.picked&1);
    CHECK(smc_lock_toggle()); CHECK(smc.picked&1); CHECK(smc_lock_toggle()); CHECK(smc.picked&1);
    tap_review(0x61); smc_frame(); CHECK(smc.picked&1);
    puts("PASS same-target Pause and knob-bank change retain fader pickup");
    setup(V_HOME,PART_303A); tap_review(14);
    while(ui.list_sel+1<ui.list_n && !(list_rows[ui.list_sel].kind==R_ACT && list_rows[ui.list_sel].a==ACT_RESET)) ui.list_sel++;
    pref_t before_ask=smc_target(0); CHECK(before_ask.kind!=R_NONE); CHECK(smc_lock_toggle());
    tap_review(5); CHECK(ui.overlay==O_ASK); tap_review(6); CHECK(ui.overlay==O_LIST);
    printf("locked canceled confirmation: target_kind=%u expected=%u\n",smc_target(0).kind,before_ask.kind);
    if(!smc_same(before_ask,smc_target(0))) { puts("FAIL canceled confirmation permanently captures parked targets"); failures++; }
    tap_review(5); CHECK(ui.overlay==O_ASK); tap_review(5);
    CHECK(!smc.locked); CHECK(!smc.nav_down); CHECK(!smc.select_down);
    puts("PASS confirmed project reset clears lock and held context");
    setup(V_MIX,PART_303A); CHECK(cur_page()==0);
    tap_review(31); CHECK(cur_page()==3); tap_review(31); CHECK(cur_page()==0);
    /* A keyboard transition starts a new shortcut gesture even on same view. */
    *page_of(V_MIX)=3; build_pages(); (void)smc_knob_target(0);
    *page_of(V_MIX)=0; build_pages(); tap_review(31); CHECK(cur_page()==3);
    tap_review(29); CHECK(ui.view==V_FX && cur_page()==0);
    tap_review(30); CHECK(cur_page()==1); tap_review(30); CHECK(cur_page()==0);
    tap_review(29); CHECK(cur_page()==0);
    puts("PASS shortcut identity, distinct first MASTER/reverb/delay selection and keyboard-context reset");
    for (int kit=0;kit<NKIT;kit++) {
        setup(V_SEQ,kit); ui.sel[kit]=8; build_pages(); int count=smc_bank_count();
        for(int bank=0;bank<count;bank++) {
            smc.bank=bank;
            if(smc_knob_target(0).kind==R_NONE) {
                printf("inactive selected knob preview: kit%d bank%d page_target_kind%u fader_owner%d\n",kit,bank,smc_page_target(0).kind,smc_fader_of(smc_page_target(0)));
                smc_ui_begin(0,1); ui_frame(); shot(kit ? "review-empty-808-bank" : "review-empty-909-bank");
            }
        }
    }
    printf("review boundary failures=%d assertions=%u\n",failures,checks);
    return failures ? 1 : 0;
}
