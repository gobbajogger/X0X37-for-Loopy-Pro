/* SPDX-License-Identifier: GPL-3.0-only */
/* Two physical-peer contracts through the complete production UI/engine/SMC.
 * Radio is substituted; this does not establish physical two-device behavior. */
#include <stdint.h>
#include <string.h>
static uint8_t out[2][512][3];
static unsigned out_n[2], reject_peer = 2;
int plat_ble_smc_send_slot(unsigned peer, const uint8_t *m, uint8_t n)
{
    if (peer >= 2 || n != 3 || peer == reject_peer) return 0;
    if (out_n[peer] < 512) memcpy(out[peer][out_n[peer]], m, 3);
    out_n[peer]++;
    return 1;
}
int plat_ble_smc_send(const uint8_t *m, uint8_t n) { return plat_ble_smc_send_slot(0, m, n); }
static uint8_t solo_parts, fx_mask, fx_values[8];
static uint16_t solo_drums[2];
static int require_zero_fx;
#define engine_smc_solo_targets track_solo_targets
#define engine_smc_busfx track_busfx
#define SMC_SLOT_SEND_IMPLEMENTED 1
#define SMC_UI_SEND_IMPLEMENTED 1
#define SMC_UI_MAIN smc_ui_baseline_main
#include "smc_ui_test.c"

#undef engine_smc_solo_targets
#undef engine_smc_busfx
extern void engine_smc_solo_targets(uint8_t,uint16_t,uint16_t);
extern void engine_smc_busfx(uint8_t,const uint8_t[8]);
void track_solo_targets(uint8_t p,uint16_t a,uint16_t b)
{ solo_parts=p;solo_drums[0]=a;solo_drums[1]=b;engine_smc_solo_targets(p,a,b); }
void track_busfx(uint8_t down,const uint8_t effects[8])
{ CHECK(!require_zero_fx || !down); fx_mask=down;if(effects)memcpy(fx_values,effects,8);engine_smc_busfx(down,effects); }


static void pair_setup(void)
{
    setup(V_HOME, PART_909);
    smc_ble_link_slot(1, 1); smc_poll();
    CHECK(smc_states[0].up && smc_states[1].up);
    memset(out_n, 0, sizeof out_n); reject_peer = 2;
}
static void peer_queue(unsigned peer, uint8_t st, uint8_t a, uint8_t b)
{ const uint8_t m[3] = {st,a,b}; smc_ble_midi_slot(peer,m,3); }
static void peer_msg(unsigned peer, uint8_t st, uint8_t a, uint8_t b)
{ peer_queue(peer,st,a,b); smc_poll(); CHECK(smc_slot == 0); }
static void peer_lock(unsigned peer)
{ peer_msg(peer,0x90,0x5D,127); peer_msg(peer,0x80,0x5D,0); }
static unsigned note_writes(unsigned peer, unsigned note, unsigned value)
{
    unsigned n = 0;
    for (unsigned i = 0; i < out_n[peer] && i < 512; i++)
        n += out[peer][i][0] == 0x90 && out[peer][i][1] == note && out[peer][i][2] == value;
    return n;
}
static void independent_targets(void)
{
    pair_setup();
    smc_view(V_MIX,1); ui.sel[0]=0; build_pages(); peer_lock(0);
    /* Peer 0 is locked: its page buttons move only its own page. Move the
     * shared keyboard page to 808 MIX before peer 1 captures it. */
    smc_view(V_MIX,-1); *page_of(V_MIX)=2; ui.sel[1]=8; build_pages(); peer_lock(1);
    CHECK(smc_states[0].locked && smc_states[1].locked);
    CHECK(smc_states[0].lock_kit == 1 && smc_states[0].lock_bank == 0);
    CHECK(smc_states[1].lock_kit == 2 && smc_states[1].lock_bank == 8);
    pref_t a=smc_states[0].lock_fader[0], b=smc_states[1].lock_fader[0];
    CHECK(!smc_same(a,b));
    pref_set(a,64); pref_set(b,96); smc_frame();
    smc_view(V_HOME,0); build_pages();
    smc_slot=0; unsigned av=smc_scaled(a); smc_slot=1; unsigned bv=smc_scaled(b); smc_slot=0;
    peer_msg(0,0xE0,0,av); CHECK(smc_states[0].picked&1); CHECK(!(smc_states[1].picked&1));
    peer_msg(0,0xE0,0,0); CHECK(pref_get(a)==0); CHECK(pref_get(b)==96);
    peer_msg(1,0xE0,0,bv); CHECK(smc_states[1].picked&1);
    peer_msg(1,0xE0,0,0); CHECK(pref_get(b)==0); CHECK(smc_ui.peer==1);
    smc_ui_draw(); CHECK(smc_slot==0); CHECK(smc_ui.peer==1);
    /* Unlocking peer zero cannot change peer one's captured kit or held edge. */
    peer_msg(0,0x90,0x5D,127); now_ms+=900; smc_poll();
    CHECK(!smc_states[0].locked && smc_states[1].locked);
    CHECK(smc_states[1].lock_kit==2 && smc_states[1].lock_bank==8);
}
static void independent_lifecycle(void)
{
    for (unsigned victim=0;victim<2;victim++) {
        unsigned other=1-victim;
        pair_setup();
        peer_msg(victim,0x90,8,127); peer_msg(other,0x90,9,127);
        CHECK(smc_states[victim].solo && smc_states[other].solo);
        CHECK(solo_parts==(smc_states[victim].solo|smc_states[other].solo));
        uint8_t held=smc_states[other].solo;
        for(unsigned i=0;i<SMC_Q;i++) peer_queue(victim,0xB0,16,0);
        peer_queue(victim,0x80,8,0); /* lost release must clear only its owner */
        smc_poll(); CHECK(!smc_states[victim].solo); CHECK(smc_states[other].solo==held); CHECK(solo_parts==held);
        peer_msg(victim,0x90,8,127);
        smc_ble_link_slot(victim,0); smc_ble_link_slot(victim,1);
        peer_queue(victim,0x90,10,127); smc_poll();
        CHECK(smc_states[other].up && smc_states[other].solo==held);
        CHECK(smc_states[victim].solo_target[0]==0 && smc_states[victim].solo_target[2]);
        /* A saturated other peer cannot take this peer's ring capacity. */
        for(unsigned i=0;i<SMC_Q;i++) peer_queue(victim,0xB0,16,0);
        peer_queue(other,0x80,9,0); smc_poll(); CHECK(!smc_states[other].solo);
        peer_queue(0,0x90,8,127); peer_queue(1,0x90,9,127);
        smc_project_reset(); smc_poll(); CHECK(!smc_project_active());
        CHECK(smc_inputs[0].qr==smc_inputs[0].qw && smc_inputs[1].qr==smc_inputs[1].qw);
        CHECK(!smc_states[0].locked && !smc_states[1].locked);
    }
}
static void independent_feedback(void)
{
    pair_setup(); peer_lock(0); memset(smc_states[0].known,0,sizeof smc_states[0].known); smc_frame();
    CHECK(note_writes(0,0x5D,127)); CHECK(out_n[0] && out_n[1]);
    /* Blocked peer zero's TX must not starve peer one or advance its own cache. */
    memset(smc_states[0].known,0,sizeof smc_states[0].known);
    memset(smc_states[1].known,0,sizeof smc_states[1].known);
    memset(out_n,0,sizeof out_n); reject_peer=0; smc_frame();
    CHECK(out_n[0]==0 && out_n[1]>0); CHECK(!smc_states[0].known[0]);
    reject_peer=2; smc_frame(); CHECK(out_n[0]>0 && smc_states[0].known[0]);
    /* Hosted channel sixteen is deliberately attached to peer zero, not the
     * peer left selected by an interrupt or the last physical control. */
    proj.set.bt|=PROJ_BT_SMC_USER;
    peer_msg(1,0x90,0x5D,127); peer_msg(1,0x80,0x5D,0);
    CHECK(smc_states[1].locked);
    CHECK(smc_midi_host(9u|0x9Fu<<8|0x5Du<<16|127u<<24)); smc_poll();
    CHECK(smc_states[0].lock_down==2 && !smc_states[1].lock_down);
}
static void independent_fx(void)
{
    pair_setup(); smc_view(V_FX,0); build_pages();
    peer_msg(0,0x90,24,127); peer_msg(1,0x90,25,127);
    CHECK((fx_mask&3)==3);
    peer_msg(0,0x80,24,0); CHECK(!(fx_mask&1) && (fx_mask&2));
    smc_ble_link_slot(0,0);smc_poll();CHECK(fx_mask&2);
    peer_msg(1,0x80,25,0);CHECK(!fx_mask);
    smc_ble_link_slot(0,1);smc_poll();
    peer_msg(0,0x90,24,127);peer_msg(1,0x90,24,127);
    smc_states[1].fx_effect[0]=smcfx_cycle(0,smc_states[0].fx_effect[0]);
    smc_slot=1;smc_fx_publish();smc_slot=0;
    /* A held FX SELECT plays the temporary low-pass on every peer; the
     * saved bank effects stay unchanged (3f35de0f). */
    CHECK(fx_values[0]==SMCFX_LP);
    CHECK(smc_states[1].fx_effect[0]==smcfx_cycle(0,smc_states[0].fx_effect[0]));
    peer_msg(0,0x80,24,0);CHECK(fx_mask&1);CHECK(fx_values[0]==SMCFX_LP);
    peer_msg(1,0x80,24,0);CHECK(!fx_mask);
    peer_msg(0,0x90,24,127);peer_msg(1,0x90,25,127);CHECK((fx_mask&3)==3);
    require_zero_fx=1;smc_project_reset();require_zero_fx=0;CHECK(!fx_mask);
}
static void queue_wrap_matrix(void)
{
    pair_setup();
    for(unsigned start=0;start<256;start++) for(unsigned victim=0;victim<2;victim++) {
        unsigned other=1-victim;
        smc_project_reset(); smc_view(V_HOME,0); build_pages();
        for(unsigned peer=0;peer<2;peer++) {
            smc_inputs[peer].qw=smc_inputs[peer].qr=(uint8_t)start;
            smc_inputs[peer].link_published=smc_inputs[peer].link_seen=0xFFFFFE01u;
            smc_states[peer].up=1;
            smc_inputs[peer].release_lost[0]=smc_inputs[peer].release_seen[0]=UINT32_MAX;
        }
        peer_msg(other,0x90,9,127);uint8_t held=smc_states[other].solo;
        peer_queue(victim,0x90,8,127);
        for(unsigned i=1;i<SMC_Q;i++)peer_queue(victim,0xB0,16,0);
        peer_queue(victim,0x80,8,0);smc_poll();
        CHECK(!smc_states[victim].solo && smc_states[other].solo==held && solo_parts==held);
        CHECK(smc_inputs[victim].release_lost[0]==0 && smc_inputs[victim].release_seen[0]==0);
        peer_queue(victim,0x90,8,127);smc_ble_link_slot(victim,0);smc_ble_link_slot(victim,1);
        peer_queue(victim,0x90,10,127);smc_poll();
        CHECK(!smc_states[victim].solo_target[0] && smc_states[victim].solo_target[2]);
        CHECK(smc_states[other].up && smc_states[other].solo==held);
        CHECK(smc_inputs[victim].qr==smc_inputs[victim].qw);
    }
    smc_project_reset();proj.set.bt|=PROJ_BT_SMC_USER;
    smc_slot=1;CHECK(smc_midi_host(9u|0x9Fu<<8|8u<<16|127u<<24));smc_slot=0;smc_poll();
    CHECK(smc_states[0].solo_target[0] && !smc_states[1].solo_target[0]);
}
int main(void)
{
    independent_targets(); independent_lifecycle(); independent_feedback(); independent_fx(); queue_wrap_matrix();
    printf("SMC two-peer production integration PASS: %u checks\n",checks);
    return 0;
}
