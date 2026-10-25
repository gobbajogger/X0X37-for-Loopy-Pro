/* SPDX-License-Identifier: GPL-3.0-only */
/* Real production UI/engine/SMC queues and framebuffer; no device I/O. */
#define SMC_UI_MAIN smc_original_ui_main
#include "smc_ui_test.c"
static void tap(int note) { message(0x90, note, 127); message(0x80, note, 0); }
static void keyboard(int view, int part)
{ ui.part = part; ui.mixsel = part; set_view(view); build_pages(); smc_ui_cancel(); }
static void test_lock_navigation(void)
{
    for (int kit = 0; kit < NKIT; kit++) {
        setup(V_SEQ, kit);
        pref_t f = smc_target(0), k = smc_knob_target(0);
        pref_set(f, 70); smc_frame(); fader(0, smc_scaled(f)); CHECK(smc.picked & 1);
        CHECK(smc_lock_toggle()); smc_frame(); CHECK(smc.picked & 1);
        keyboard(V_PART, PART_303B); ui.sel[kit] = 9; build_pages();
        CHECK(smc_same(f, smc_target(0))); CHECK(smc_same(k, smc_knob_target(0)));
        int before = pref_get(k); knob(0, 1); CHECK(pref_get(k) > before);
        tap(0x2F); CHECK(smc.locked && smc.lock_bank == 8);
        CHECK(smc_target(0).b == 8); CHECK(smc_target(3).kind == R_NONE);
        smc_frame(); CHECK(!(smc.picked & 1));
        tap(0x2E); CHECK(smc.lock_bank == 0);
        tap(0x31); CHECK(smc.lock_sel[kit] == 1);
        int rec = ui.rec; tap(7); CHECK(ui.rec != rec && ui.sel[kit] == 7);
        CHECK(smc.lock_sel[kit] == 7); /* R8 remains voice8 REC */
        tap(23); CHECK(track_muted(kit, 7));
        message(0x90, 15, 127); CHECK(smc.drum_solo[kit] & (1u << 7));
        tap(0x2F); CHECK(smc.lock_bank == 8);
        message(0x80, 15, 0); CHECK(!smc_project_active());
        CHECK(smc_target(7).kind == R_NONE); tap(7); CHECK(smc.lock_sel[kit] == 8);
        tap(0x2E); tap(24); CHECK(smc.lock_sel[kit] == 0);
        keyboard(V_PART, PART_303A);
        tap(0x63); CHECK(smc.lock_view == V_SEQ && smc.lock_part == kit);
        CHECK(smc_same(f, smc_target(0)));
    }
    puts("PASS keyboard isolation vs locked SMC bank/channel/REC/SELECT/navigation, held Solo and empty lanes");
}
static void test_banks(void)
{
    setup(V_HOME, PART_303A);
    pref_t f[8]; for (int n=0;n<8;n++) f[n]=smc_target(n);
    int count = smc_bank_count(); CHECK(count > 3);
    tap(0x61); CHECK(smc.bank == 1); CHECK(smc_knob_target(0).c == MX_REV);
    CHECK(smc_knob_target(5).kind == R_NONE);
    tap(0x61); CHECK(smc.bank == 2); CHECK(smc_knob_target(4).c == MX_DLY);
    for(int n=0;n<8;n++) CHECK(smc_same(f[n],smc_target(n)));
    for(int i=2;i<count;i++) tap(0x61); CHECK(smc.bank==0);
    tap(0x60); CHECK(smc.bank == count-1); tap(0x61); CHECK(!smc.bank);
    for(int kit=0;kit<NKIT;kit++) {
        setup(V_SEQ,kit); CHECK(smc_lock_toggle()); keyboard(V_PART, PART_303B);
        tap(0x61); CHECK(smc.bank==1); CHECK(name_is(pref_desc(smc_knob_target(0)),"Rev"));
        tap(0x61); CHECK(name_is(pref_desc(smc_knob_target(0)),"Dly"));
        tap(0x61); CHECK(smc.bank==3); CHECK(smc_knob_target(0).kind == R_ENG);
        knob(0,1); ui_frame(); shot(kit ? "navigation-808-voice-bank" : "navigation-909-voice-bank");
    }
    /* Every editable descriptor on sound/FX/generator pages and MORE is reachable. */
    int views[]={V_PART,V_FX,V_GEN,V_MIX,V_SONG};
    for(unsigned v=0;v<sizeof views/sizeof *views;v++) {
        setup(views[v],PART_303A);
        if(views[v]==V_MIX) { *page_of(V_MIX)=3; build_pages(); }
        smc_bank_sync(); count=smc_bank_count();
        for(int i=0;smc_extra_target(i).kind!=R_NONE;i++) {
            pref_t want=smc_extra_target(i); int found=0;
            for(int bank=1;bank<count;bank++) { smc.bank=bank;
                for(int n=0;n<8;n++) if(smc_same(want,smc_knob_target(n))) found=1;
            }
            CHECK(found);
        }
    }
    setup(V_PART,PART_303A); tap(0x61); CHECK(smc.bank==1);
    knob(5,1); ui_frame(); shot("navigation-303-extra-bank");
    keyboard(V_FX,PART_303A); (void)smc_knob_target(0); CHECK(smc.bank==0);
    puts("PASS context bank forward/back wrap, exact sends, descriptor and MORE reachability, fixed faders");
}
static void test_modes(void)
{
    setup(V_HOME,PART_303A); tap(15); CHECK(ui.view==V_PART); CHECK(cur_page()==0);
    message(0x90,15,127); CHECK(cur_page()==1);
    message(0x90,15,127); CHECK(cur_page()==1); message(0x80,15,0);
    tap(15); CHECK(cur_page()==2); tap(15); CHECK(cur_page()==0);
    tap(0x62); CHECK(cur_page()==2); tap(0x63); CHECK(cur_page()==0);
    tap(29); CHECK(ui.view==V_FX && cur_page()==0); tap(29); CHECK(cur_page()==1);
    tap(29); CHECK(cur_page()==0);
    tap(31); CHECK(ui.view==V_MIX && cur_page()==3); tap(31); CHECK(cur_page()==0);
    tap(31); CHECK(cur_page()==1); /* subsequent distinct presses cycle MIX */
    setup(V_MIX, PART_303A); tap(31); CHECK(cur_page()==3); /* first selects MASTER */
    tap(31); CHECK(cur_page()==0);
    CHECK(smc_lock_toggle()); keyboard(V_PART,PART_303B);
    tap(15); CHECK(smc.lock_view==V_PART && smc.lock_part==PART_303A);
    tap(15); CHECK(smc.lock_page==1); knob(0,1); ui_frame(); shot("navigation-locked-sound-page");
    setup(V_SEQ,PART_303A); tap(5); CHECK(ui.overlay==O_LIST);
    int bank=smc.bank; tap(0x61); CHECK(ui.overlay==O_LIST && smc.bank==bank);
    setup(V_SEQ,PART_303A); clip_grid_mode=1; ck.pg=1;
    tap(0x61); CHECK(smc.bank==0); CHECK(ck.pg!=1);
    puts("PASS first-select/repeat-page, held suppression, page wrap, locked context, menu and clip ownership");
}
int main(void)
{
    test_lock_navigation(); test_banks(); test_modes();
    printf("smc_navigation: PASS (%u production-linked checks)\n",checks); return 0;
}
