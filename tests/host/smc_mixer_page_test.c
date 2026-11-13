/* SPDX-License-Identifier: GPL-3.0-only */
/* Each peer navigates existing mixer descriptors without touching keyboard pages. */
#define BLEC_TWO_PEER 1
#define SMC_UI_MAIN legacy_ui_main
#include "smc_ui_test.c"
#undef SMC_UI_MAIN
int main(void)
{
    setup(V_MIX,PART_909);
    *page_of(V_MIX)=3;build_pages();
    CHECK(smc_page_now()==0 && cur_page()==3);
    smc_page_set(1);
    CHECK(smc_page_now()==1 && cur_page()==3 && smc_drum_mix()==PART_909);
    pref_t first=smc_target(0);
    smc_ble_link_slot(1,1);smc_poll();
    smc_slot=1;smc_view(V_MIX,2);smc.sel[1]=1;
    CHECK(smc_page_now()==2 && smc_led(25) && !smc_led(24));
    CHECK(!smc_same(first,smc_target(0)));
    CHECK(smc_lock_toggle());
    pref_t peer1=smc_target(0);
    smc_slot=0;
    CHECK(smc_page_now()==1 && smc_led(24));
    CHECK(smc_lock_toggle());
    pref_t held=smc_target(0);
    const int views[]={V_PART,V_FX,V_GEN,V_SONG};
    for(unsigned i=0;i<sizeof views/sizeof *views;i++) {
        ui.part=PART_303A;set_view(views[i]);build_pages();
        int keyboard=cur_page();
        CHECK(smc_same(held,smc_target(0)) && smc_page_now()==1);
        smc_page_set(2);CHECK(smc_page_now()==2 && smc_drum_mix()==PART_808);
        CHECK(cur_page()==keyboard && ui.view==views[i]);
        smc_page_set(1);CHECK(smc_same(held,smc_target(0)));
        smc_slot=1;CHECK(smc_same(peer1,smc_target(0)) && smc_page_now()==2);smc_slot=0;
    }
    /* Locked BANK arrows still retarget the captured kit. Knob assignment,
     * relative edits and pickup faders remain live inside that own context. */
    smc_knob_bank(1);
    pref_t assignment = smc_knob_target(0);
    smc_knob_bank(1);
    CHECK(!smc_same(assignment, smc_knob_target(0)));
    message(0x90, 0x2F, 127); message(0x80, 0x2F, 0);
    CHECK(smc_drum_track(PART_909, 0)==8 && smc_page_now()==1);
    pref_t locked_knob=smc_knob_target(0), locked_fader=smc_target(0);
    CHECK(locked_fader.b==8 && locked_knob.b==8);
    const x0x_param_t *kd=pref_desc(locked_knob);
    pref_set(locked_knob,kd->max/2);
    int old_knob=pref_get(locked_knob);
    knob(0,1); CHECK(pref_get(locked_knob)>old_knob);
    pref_set(locked_fader,64);smc_frame();
    fader(0,smc_scaled(locked_fader)); fader(0,0);
    CHECK(pref_get(locked_fader)==0 && (smc.picked&1));
    int own_bank=smc_drum_track(PART_909,0);
    ui.part=PART_303B;set_view(V_SEQ);build_pages();
    CHECK(smc_page_now()==1 && smc_drum_track(PART_909,0)==own_bank);
    smc_page_set(2); CHECK(smc_page_now()==2);
    CHECK(smc_lock_toggle());
    CHECK(smc_page_now()==2 && ui.view==V_SEQ && ui.part==PART_303B);
    smc_page_set(1);
    for(int bank=0;bank<2;bank++) {
        smc_select_drum(0,bank*8);
        unsigned tracks=0;
        for(int i=0;i<8;i++) {
            pref_t f=smc_target(i),k=smc_knob_target(i);
            if(bank && i>=3){CHECK(f.kind==R_NONE && k.kind==R_NONE);continue;}
            CHECK(f.b==bank*8+i && f.a==T_909);tracks|=1u<<f.b;
            CHECK(!smc_same(k,f));
            for(int j=0;j<i;j++)CHECK(!smc_same(k,smc_knob_target(j)));
        }
        CHECK(tracks==(bank?0x700u:0xffu));
        for(int b=0;b<3;b++){smc_knob_bank(1);for(int i=0;i<8;i++)
            for(int j=0;j<8;j++)CHECK(smc_knob_target(i).kind==R_NONE || !smc_same(smc_knob_target(i),smc_target(j)));}
    }
    /* Approved #737: a short Pause press while locked peeks the locked
     * page for SMC_PEEK_MS, then restores the keyboard page. Lock stays on. */
    int keyboard=cur_page(),view=ui.view;
    smc_lock_button(1);smc_lock_button(0);CHECK(smc.locked);
    smc_lock_button(1);smc_lock_button(0);
    CHECK(smc.locked && smc_peek->on && ui.view==smc.lock_view && cur_page()==smc.lock_page);
    now_ms+=SMC_PEEK_MS+1;smc_poll();
    CHECK(smc.locked && !smc_peek->on && cur_page()==keyboard && ui.view==view);
    /* Long Pause press still unlocks and shows no peek. */
    smc_lock_button(1);now_ms+=900;smc_poll();smc_lock_button(0);
    CHECK(!smc.locked && !smc_peek->on && cur_page()==keyboard && ui.view==view);
    printf("smc_mixer_page: PASS (%u checks; peers, banks, lock, keyboard isolation)\n",checks);
    return 0;
}
