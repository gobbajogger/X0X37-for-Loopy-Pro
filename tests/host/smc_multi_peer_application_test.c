/* SPDX-License-Identifier: GPL-3.0-only */
/* Full production UI/project/engine, decoder, locks, pickup and feedback.
 * The existing host fixture supplies hardware substitutes; no device claim. */
#define SMC_UI_MAIN smc_ui_single_peer_main
#include "smc_ui_test.c"
static void peer_message(uint8_t peer, uint8_t st, uint8_t a, uint8_t b)
{
    const uint8_t msg[] = {st,a,b};
    smc_ble_midi_peer(peer,msg,sizeof msg); smc_poll();
}
static pref_t peer_fader(uint8_t peer, int n)
{
    uint8_t saved = smc_peer; smc_peer=peer;
    pref_t r=smc_target(n); smc_peer=saved; return r;
}
static pref_t peer_knob(uint8_t peer, int n)
{
    uint8_t saved = smc_peer; smc_peer=peer;
    pref_t r=smc_knob_target(n); smc_peer=saved; return r;
}
static void two_links(void)
{
    setup(V_SEQ,PART_909);
    smc_ble_link_peer(1,1);smc_poll();
}
static void navigate_peer_target(int kit,int bank)
{
    ui.part=(uint8_t)kit;ui.sel[kit]=(uint8_t)bank;
    set_view(V_SEQ);build_pages();smc_ui_cancel();
}
static void distinct_locks_pickup_and_feedback(void)
{
    two_links();navigate_peer_target(PART_909,8);
    CHECK(smc_peer_lock(0,1));
    pref_t a=peer_fader(0,0),ak=peer_knob(0,0);
    navigate_peer_target(PART_808,0);CHECK(smc_peer_lock(1,1));
    pref_t b=peer_fader(1,0),bk=peer_knob(1,0);
    CHECK(!smc_same(a,b)&&!smc_same(ak,bk));
    ui.part=PART_303A;set_view(V_HOME);build_pages();
    CHECK(smc_same(a,peer_fader(0,0))&&smc_same(b,peer_fader(1,0)));
    pref_set(a,64);pref_set(b,64);smc_frame();
    peer_message(0,0xe0,0,0);peer_message(1,0xe0,0,127);
    CHECK(!smc_states[0].picked&&!smc_states[1].picked);
    peer_message(0,0xe0,0,127);
    CHECK(pref_get(a)==127&&pref_get(b)==64);
    CHECK((smc_states[0].picked&1)&&!(smc_states[1].picked&1));
    CHECK(smc_states[1].last[0]==127);
    peer_message(1,0xe0,0,0);
    CHECK(pref_get(b)==0&&pref_get(a)==127);
    CHECK((smc_states[1].picked&1)&&smc_states[0].last[0]==127);
    pref_set(ak,48);pref_set(bk,48);
    peer_message(0,0xb0,16,1);CHECK(pref_get(ak)>48&&pref_get(bk)==48);
    uint32_t t=smc_states[0].knob_t;uint8_t run=smc_states[0].knob_run;
    peer_message(1,0xb0,16,1);CHECK(pref_get(bk)>48);
    CHECK(smc_states[0].knob_t==t&&smc_states[0].knob_run==run);
    CHECK(smc_ui.peer==1&&smc_ui_active());
    ui_frame();CHECK(smc_peer==0); /* drawing selects the touched peer, then restores context */
    smc_ble_link_peer(0,0);smc_poll();CHECK(smc_ui.peer==1&&smc_ui_active());
    CHECK(smc_peer_locked(0)&&smc_peer_locked(1));
    CHECK(smc_same(a,peer_fader(0,0))&&smc_same(b,peer_fader(1,0)));
    smc_ble_link_peer(0,1);smc_poll();
    CHECK(!smc_states[0].input_seen&&!smc_states[0].picked);
    CHECK(smc_states[1].input_seen&&smc_states[1].picked);
    /* Per-peer feedback cache follows each captured target, including after remote edits. */
    pref_set(a,33);pref_set(b,101);smc_frame();
    CHECK(smc_states[0].value[0]==33&&smc_states[1].value[0]==101);
    CHECK(!smc_states[0].picked&&!smc_states[1].picked);
    CHECK(!smc_peer_lock(2,1)&&!smc_peer_locked(255));
    puts("PASS full production: two captured kits/banks, independent fader crossing/knob acceleration, display owner and feedback");
}
static void same_target_and_solo(void)
{
    two_links();navigate_peer_target(PART_909,8);
    CHECK(smc_peer_lock(0,1)&&smc_peer_lock(1,1));
    pref_t r=peer_fader(0,0);CHECK(smc_same(r,peer_fader(1,0)));
    pref_set(r,64);smc_frame();
    peer_message(0,0xe0,0,0);peer_message(1,0xe0,0,127);
    peer_message(0,0xe0,0,65);CHECK(pref_get(r)==65);
    CHECK((smc_states[0].picked&1)&&!(smc_states[1].picked&1));
    peer_message(1,0xe0,0,64);CHECK(pref_get(r)==64);
    CHECK(smc_states[0].last[0]==65&&smc_states[1].last[0]==64);
    peer_message(0,0x90,8,127);peer_message(1,0x90,8,127);
    CHECK(smc_drum_solo_all[0]==(1u<<8));
    peer_message(0,0x80,8,0);CHECK(smc_drum_solo_all[0]==(1u<<8));
    smc_ble_link_peer(0,0);smc_poll();CHECK(smc_drum_solo_all[0]==(1u<<8));
    peer_message(1,0x80,8,0);CHECK(!smc_project_active());
    smc_ble_link_peer(0,1);smc_poll();
    /* Intentional navigation on A replaces only A's captured target map. */
    peer_message(0,0x90,0x2e,127);peer_message(0,0x80,0x2e,0);
    CHECK(!smc_same(peer_fader(0,0),peer_fader(1,0)));
    CHECK(smc_states[0].lock_bank==0&&smc_states[1].lock_bank==8);
    peer_message(0,0x90,8,127);peer_message(1,0x90,8,127);
    CHECK(smc_drum_solo_all[0]==((1u<<0)|(1u<<8)));
    smc_ble_link_peer(0,0);smc_poll();CHECK(smc_drum_solo_all[0]==(1u<<8));
    smc_project_reset();CHECK(!smc_project_active());
    CHECK(!smc_peer_locked(0)&&!smc_peer_locked(1));
    puts("PASS full production: same-target pickup, same/different Solo union, per-peer intentional navigation and disconnect/reset");
}
int main(void)
{
    distinct_locks_pickup_and_feedback();same_target_and_solo();
    printf("smc_multi_peer_application: PASS (%u checks)\n",checks);
    return 0;
}
