/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN legacy_ui_main
#include "smc_ui_test.c"
#undef SMC_UI_MAIN

static void commit_bank(int bank)
{
    CHECK(engine_bass_select_preset(0,bank,0,&proj.sound)==0);
    for(int n=0;n<1024 && engine_bass_preset_pending(0);n++) {
        int32_t audio[512];engine_render(audio,256);
    }
    CHECK(engine_bass_bank(0)==bank);
}

int main(void)
{
    if(getenv("SMC_ENGINE_STALE_ONLY"))goto stale;
    setup(V_PART,PART_303A);commit_bank(ORIGINAL_SOUND_CHICAGO);build_pages();
    *page_of(V_PART)=(uint8_t)(pg.n-1);ui_frame();
    smc_page_set(smc_page_count()-1);
    CHECK(smc_knob_target(0).kind==R_BENGINE);
    uint8_t saved[NVIEWS][NPARTS];memcpy(saved,ui.page,sizeof saved);
    knob(0,1);
    for(int n=0;n<1024 && engine_bass_preset_pending(0);n++) {
        int32_t audio[512];engine_render(audio,256);
    }
    CHECK(engine_bass_bank(0)==ORIGINAL_SOUND_MIAMI);
    ui_frame();CHECK(!memcmp(saved,ui.page,sizeof saved));
    now_ms+=1300;ui_frame();CHECK(!memcmp(saved,ui.page,sizeof saved));
stale:
    /* A normal synth detent must never take the keyboard stale-page path. */
    setup(V_PART,PART_303A);commit_bank(ORIGINAL_SOUND_CHICAGO);build_pages();
    *page_of(V_PART)=(uint8_t)(pg.n-1);open_list_of_pages();
    uint8_t old_page[NVIEWS][NPARTS];memcpy(old_page,ui.page,sizeof old_page);
    unsigned list_sel=ui.list_sel,list_top=ui.list_top,list_n=ui.list_n;
    commit_bank(ORIGINAL_SOUND_MIAMI);smc_page_set(0);
    pref_t target=smc_knob_target(0);CHECK(target.kind==R_ENG && target.a==T_303);
    pref_set(target,20);int old=pref_get(target);knob(0,1);
    CHECK(pref_get(target)>old);
    CHECK(!memcmp(old_page,ui.page,sizeof old_page));
    CHECK(ui.list_sel==list_sel && ui.list_top==list_top && ui.list_n==list_n);
    printf("smc_popup_engine: PASS (%u checks; deferred refresh and stale keyboard pages)\n",checks);
    return 0;
}
