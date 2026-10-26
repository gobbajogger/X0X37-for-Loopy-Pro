/* SPDX-License-Identifier: GPL-3.0-only */
/* Combined real-queue routing + fake-clock acceptance; no device access. */
#define SMC_UI_MAIN smc_ui_existing_main
#include "smc_ui_test.c"
#undef SMC_UI_MAIN
static void tap(int note) { message(0x90,note,127); message(0x80,note,0); }
static void bank_only(void)
{
    setup(V_PART, PART_303A);
    int view=ui.view,page=cur_page(); pref_t f=smc_target(0), first=smc_knob_target(0);
    tap(0x61); CHECK(ui.view==view && cur_page()==page); CHECK(smc_same(f,smc_target(0)));
    int mapped=0;
    for(int n=0;n<4;n++) {
        pref_t r=smc_knob_target(n);
        if(r.kind!=R_NONE) {
            CHECK(smc_same(r,pg.r[1][n]));
            const x0x_param_t *d=pref_desc(r);
            if(d && d->max>24) {
                int old=pref_get(r); knob(n,old<d->max?1:-1); CHECK(pref_get(r)!=old); mapped++;
                char label[24]; smc_ui_label(r,label); CHECK(label[0]);
            }
        }
    }
    CHECK(mapped); tap(0x60); CHECK(ui.view==view && cur_page()==page);
    CHECK(smc_same(first,smc_knob_target(0)));
    puts("PASS Up/Down: mapping bank only, opposite direction restores, real target edit and label");
}
static void modes(void)
{
    setup(V_PART, PART_303A); tap(15); CHECK(ui.view==V_PART && cur_page()==0);
    message(0x90,15,127); CHECK(cur_page()==1);
    message(0x90,15,127); CHECK(cur_page()==1); message(0x80,15,0);
    int wrapped=0;
    for(int i=0;i<32;i++) { tap(15); if(ui.view==V_PART && cur_page()==0) { wrapped=1; break; } }
    CHECK(wrapped);
    tap(7); CHECK(ui.view==V_HOME); tap(7); CHECK(ui.view==V_SEQ);
    wrapped=0;
    for(int i=0;i<16;i++) { tap(7); if(ui.view==V_HOME && cur_page()==0) { wrapped=1; break; } }
    CHECK(wrapped);
    setup(V_FX,PART_303A); tap(31); CHECK(ui.view==V_MIX && cur_page()==3);
    const int pages[]={4,0,1,2,3};
    for(unsigned i=0;i<sizeof pages/sizeof *pages;i++) {
        tap(31); CHECK(ui.view==V_MIX && cur_page()==pages[i]); CHECK(!smc.fx_down);
    }
    /* Entering drums independently preserves the physical strip8 drum functions. */
    setup(V_MIX,PART_909); *page_of(V_MIX)=1; build_pages();
    message(0x90,15,127); CHECK(smc.drum_solo[0]==(1u<<7));
    CHECK(ui.view==V_MIX && cur_page()==1); message(0x80,15,0); CHECK(!smc_project_active());
    puts("PASS mode keys: repeat advance/wrap, held repeat once, MIX through drum pages, direct drum8 Solo retained");
}
static void combined_locked_fx(void)
{
    setup(V_SEQ,PART_909); CHECK(smc_lock_toggle()); pref_t f=smc_target(0);
    ui.part=PART_303A; set_view(V_FX); build_pages(); CHECK(smc_same(f,smc_target(0)));
    message(0x90,27,127); CHECK(smc.fx_down==(1u<<SMCFX_303B));
    CHECK(!strcmp(smc_button_caption(27),"LPF")); CHECK(smc_led(27)); CHECK(smc_ui_active());
    now_ms+=5000; smc_poll(); CHECK(smc_ui_active()); CHECK(smc.fx_down);
    tap(31); CHECK(ui.view==V_MIX && cur_page()==3); CHECK(!smc.fx_down && !smc.fx_owned);
    CHECK(smc.locked && smc_same(f,smc_target(0))); CHECK(smc_ui_active());
    tap(0x61); CHECK(ui.view==V_MIX && cur_page()==3); CHECK(smc_same(f,smc_target(0)));
    tap(0x61); pref_t mapped=smc_knob_target(1); CHECK(mapped.kind!=R_NONE);
    char label[24]; smc_ui_label(mapped,label); CHECK(label[0]);
    CHECK(ui.view==V_MIX && cur_page()==3); tap(0x60);
    now_ms+=5000; CHECK(smc_ui_active());
    message(0x80,27,0); CHECK(smc_ui_active()); CHECK(!smc.fx_down);
    now_ms+=1199; CHECK(smc_ui_active()); now_ms++; CHECK(!smc_ui_active());
    CHECK(smc.locked && smc_same(f,smc_target(0)));
    puts("PASS combined lock + FX + Select8 page + knob banks + long hold + last-release grace");
}
static void lifetime(void)
{
    setup(V_PART,PART_303A);
    message(0x90,0,127); int rec=ui.rec; CHECK(smc_ui_active());
    message(0x90,0,127); CHECK(ui.rec==rec);
    now_ms+=5000; CHECK(smc_ui_active());
    message(0x90,16,127); now_ms+=5000; message(0x80,0,0); CHECK(smc_ui_active());
    now_ms+=5000; CHECK(smc_ui_active()); message(0x80,16,0);
    now_ms+=1199; CHECK(smc_ui_active()); now_ms++; CHECK(!smc_ui_active());
    setup(V_HOME,PART_303A); pref_t f=smc_target(0); pref_set(f,100);
    fader(0,0); CHECK(smc_ui_active()); CHECK(!(smc.picked&1));
    now_ms+=1199; fader(0,1); CHECK(smc_ui_active()); CHECK(pref_get(f)==100);
    uint32_t until=smc_ui.until;
    now_ms+=1000; fader(0,1); CHECK(smc_ui.until==until);
    const uint8_t clock[]={0xF8}; smc_ble_midi(clock,1); smc_poll(); smc_frame();
    CHECK(smc_ui.until==until); now_ms+=199; CHECK(smc_ui_active()); now_ms++; CHECK(!smc_ui_active());
    smc_ble_midi(clock,1); smc_poll(); smc_frame(); CHECK(!smc_ui_active());
    fader(0,2); CHECK(smc_ui_active());
    now_ms=UINT32_MAX-600u; fader(0,3); now_ms+=1199; CHECK(smc_ui_active()); now_ms++; CHECK(!smc_ui_active());
    puts("PASS lifetime: overlapping buttons, last release, pickup-pending slow movement, duplicate/clock silence and timer wrap");
}
static void cleanup(void)
{
    for(int host=0;host<2;host++) {
        setup(V_PART,PART_303A); proj.set.bt|=PROJ_BT_SMC_USER;
        tap(0); message(0x90,0,127); CHECK(smc_ui_active());
        const uint8_t filler[]={0xB0,16,0},release[]={0x80,0,0};
        for(unsigned i=0;i<SMC_Q;i++) {
            if(host) CHECK(smc_midi_host(11u|0xBFu<<8|16u<<16));
            else smc_ble_midi(filler,3);
        }
        if(host) CHECK(smc_midi_host(8u|0x8Fu<<8)); else smc_ble_midi(release,3);
        smc_poll(); now_ms+=2000; CHECK(!smc_ui_active());
    }
    setup(V_FX,PART_303A); message(0x90,24,127); CHECK(smc_ui_active());
    smc_ble_link(0); smc_poll(); CHECK(!smc_ui_active() && !smc.fx_down);
    smc_ble_link(1); smc_poll(); now_ms+=5000; CHECK(!smc_ui_active());
    message(0x90,24,127); CHECK(smc_ui_active()); smc_project_reset(); CHECK(!smc_ui_active());
    puts("PASS lifetime cleanup: BLE/host lost Record release, disconnect/reconnect and project reset");
}
int main(void)
{
    const char *only=getenv("SMC_NAV_CASE");
    if(!only||!strcmp(only,"bank")) bank_only();
    if(!only||!strcmp(only,"modes")) modes();
    if(!only||!strcmp(only,"combined")) combined_locked_fx();
    if(!only||!strcmp(only,"lifetime")) lifetime();
    if(!only||!strcmp(only,"cleanup")) cleanup();
    printf("PASS SMC navigation/lifetime: %u checks\n",checks); return 0;
}
