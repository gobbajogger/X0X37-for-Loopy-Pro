/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN legacy_ui_main
#include "smc_ui_test.c"
#undef SMC_UI_MAIN
int main(void)
{
    setup(V_HOME, PART_909);
    clip_grid_mode = 1;
    CHECK(!smc_lock_toggle());
    CHECK(!smc.locked);
    CHECK(!strcmp(ui.msg,"SMC LOCK UNAVAILABLE"));
    clip_grid_mode = 0;
    setup(V_PART, PART_303A);
    CHECK(smc_lock_toggle());
    CHECK(!strcmp(ui.msg,"SMC TARGETS LOCKED"));
    CHECK(smc_page_count() > 1);
    smc_page_set(1);
    CHECK(smc.lock_page == 1);
    CHECK(smc_lock_toggle());
    CHECK(!strcmp(ui.msg,"SMC FOLLOWS PAGE"));
    CHECK(smc.mixer_page[smc.lock_view][smc_page_part(smc.lock_view, smc.lock_part)] == smc.lock_page);
    ui.help = 1;
    smc_button_apply(6, 0);
    CHECK(!ui.help);
    setup(V_HOME, PART_909);
    smc_button_apply(5, 0);
    CHECK(smc.view == V_MIX && smc_page_now() == 0);
    uint32_t context = smc_ui_context();
    smc.sel[0]++;
    CHECK(context != smc_ui_context());
    /* Locked PAGE must keep the same bank for targets, grid and musical keys. */
    setup(V_MIX,PART_909);smc.sel[PART_909]=8;smc.sel[PART_808]=0;
    smc_page_set(1);CHECK(smc_lock_toggle());smc_page_set(2);
    CHECK(smc_target(0).a==T_808 && smc_target(0).b==8);
    CHECK(smc_knob_target(0).a==T_808 && smc_knob_target(0).b==8);
    CHECK(smc_drum_track(PART_808,0)==8);
    int muted=track_muted(PART_808,8);
    message(0x90,16,127);message(0x80,16,0);
    CHECK(track_muted(PART_808,8)!=muted);
    message(0x90,0x2e,127);message(0x80,0x2e,0);
    CHECK(smc.lock_bank==0 && smc.lock_kit==PART_808+1);
    CHECK(smc_ui_active());
    CHECK(smc_target(0).b==0 && smc_knob_target(0).b==0 && smc_drum_track(PART_808,0)==0);
    /* Panel HOME remains a modifier, including the remote HOME key itself. */
    setup(V_FX,PART_303A);smc_fx_publish();held_btn=1u<<B_HOME;
    uint8_t effect=smcfx_strip_get(&smc.fx_strip,smc.fx_page,SMCFX_MDLY,SMCFX_REC);
    message(0x90,6,127);
    CHECK(smc_home_held());
    CHECK(smcfx_strip_get(&smc.fx_strip,smc.fx_page,SMCFX_MDLY,SMCFX_REC)==smcfx_cycle(SMCFX_MDLY,effect));
    CHECK(!smc.fx_down);
    message(0x80,6,0);held_btn=0;
    CHECK(!smc_home_held());
    printf("smc_audit_core: PASS (%u checks)\n", checks);
    return 0;
}
