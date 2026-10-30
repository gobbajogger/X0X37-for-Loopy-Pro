/* SPDX-License-Identifier: GPL-3.0-only */
/* Real production UI/SMC queues with synchronous ISR interleavings. No radio or
 * device I/O. These checks do not establish target IRQ timing or hardware PASS. */
#include <stdint.h>
#include <string.h>
static unsigned inject_peer, inject_solo, inject_feedback, reject_tx, tx_count[2];
void smc_ble_link_slot(unsigned, int);
void smc_ble_midi_slot(unsigned, const uint8_t *, uint8_t);
int smc_midi_host(uint32_t);
static uint32_t host_pkt(uint8_t st, uint8_t a, uint8_t b)
{ return 9u | (uint32_t)st << 8 | (uint32_t)a << 16 | (uint32_t)b << 24; }
int plat_ble_smc_send_slot(unsigned p, const uint8_t *m, uint8_t n)
{
    (void)m; (void)n;
    tx_count[p]++;
    if (inject_feedback && p == inject_peer) {
        inject_feedback = 0;
        smc_ble_link_slot(p, 0); smc_ble_link_slot(p, 1);
    }
    return !reject_tx;
}
int plat_ble_smc_send(const uint8_t *m, uint8_t n) { return plat_ble_smc_send_slot(0,m,n); }
#define engine_smc_solo_targets port_solo_targets
#define SMC_SLOT_SEND_IMPLEMENTED 1
#define SMC_UI_SEND_IMPLEMENTED 1
#define SMC_UI_MAIN smc_ui_baseline_main
#include "smc_ui_test.c"
#undef engine_smc_solo_targets
extern void engine_smc_solo_targets(uint8_t,uint16_t,uint16_t);
void port_solo_targets(uint8_t parts, uint16_t a, uint16_t b)
{
    engine_smc_solo_targets(parts,a,b);
    if (inject_solo) {
        unsigned route = inject_solo; inject_solo = 0;
        smc_ble_link_slot(inject_peer,0); smc_ble_link_slot(inject_peer,1);
        if (route == 1) {
            const uint8_t fresh[3] = {0x90,9,127};
            smc_ble_midi_slot(inject_peer,fresh,3);
        } else if(route == 2) CHECK(smc_midi_host(host_pkt(0x9F,9,127)));
    }
}
static void prepare(int user)
{
    inject_solo = inject_feedback = reject_tx = 0;
    setup(V_HOME,PART_909);
    if (user) proj.set.bt |= PROJ_BT_SMC_USER;
    smc_ble_link_slot(1,1); smc_poll();
    memset(tx_count,0,sizeof tx_count);
}
static void queue(unsigned p,uint8_t st,uint8_t a,uint8_t b)
{ const uint8_t m[3]={st,a,b}; smc_ble_midi_slot(p,m,3); }
static void fill(unsigned p)
{ for(unsigned i=0;i<SMC_Q;i++) queue(p,0xB0,16,0); }
static void peer_loss_epochs(void)
{
    for (unsigned p=0;p<2;p++) {
        prepare(1); unsigned other=1-p;
        queue(other,0x90,10,127); smc_poll();
        uint8_t held=smc_states[other].solo;
        fill(p); queue(p,0x80,8,0); queue(p,0x80,0x5D,0);
        smc_inputs[p].qr += 2; /* consumer freed slots before the ISR edge */
        smc_ble_link_slot(p,0); smc_ble_link_slot(p,1);
        queue(p,0x90,8,127); queue(p,0x90,0x5D,127); smc_poll();
        CHECK(smc_states[p].solo_target[0]); CHECK(smc_states[p].lock_down);
        CHECK(smc_states[other].solo==held);
        /* New-session loss must still clear the new-session press. */
        smc_ble_link_slot(p,0); smc_ble_link_slot(p,1);
        queue(p,0x90,8,127);
        for(unsigned i=1;i<SMC_Q;i++) queue(p,0xB0,16,0);
        queue(p,0x80,8,0); smc_poll();
        CHECK(!smc_states[p].solo); CHECK(smc_states[other].solo==held);
        /* A project reset may retire losses newer than a pending edge. */
        smc_ble_link_slot(p,0); smc_ble_link_slot(p,1);
        fill(p); queue(p,0x80,8,0); smc_project_reset();
        queue(p,0x90,8,127); smc_poll(); CHECK(smc_states[p].solo_target[0]);
    }
    prepare(1);
    CHECK(smc_midi_host(host_pkt(0x9F,8,127))); smc_poll();
    for(unsigned i=0;i<SMC_Q;i++) CHECK(smc_midi_host(host_pkt(0xBF,40,0)));
    CHECK(smc_midi_host(host_pkt(0x8F,8,0)));
    smc_ble_link_slot(0,0); smc_ble_link_slot(0,1);
    queue(0,0x90,9,127); smc_poll(); CHECK(smc_states[0].solo_target[1]);
    puts("PASS old/new link and project loss watermarks, both peers and host");
}
static void ingress_and_foreign(void)
{
    for(unsigned p=0;p<2;p++) {
        prepare(1); queue(p,0x90,8,127); smc_poll(); CHECK(smc_states[p].solo);
        for(unsigned ch=1;ch<15;ch++) {
            queue(p,(uint8_t)(0x90|ch),9,127); queue(p,(uint8_t)(0x80|ch),8,0);
        }
        smc_poll(); CHECK(smc_states[p].solo_target[0]); CHECK(!smc_states[p].solo_target[1]);
        fill(p); queue(p,0x81,8,0); smc_poll(); CHECK(smc_states[p].solo_target[0]);
        for(unsigned mode=0;mode<2;mode++) {
            prepare(mode); queue(p,0x90,8,127);
            proj.set.bt ^= PROJ_BT_SMC_USER; smc_poll(); CHECK(!smc_states[p].solo);
            queue(p,0x90,9,127); smc_poll(); CHECK(smc_states[p].solo_target[1]);
        }
        prepare(1);
        const uint8_t extended[6]={0x90,8,127,0,0,0};
        for(unsigned n=0;n<6;n++) if(n!=3) smc_ble_midi_slot(p,extended,n);
        smc_poll(); CHECK(!smc_states[p].solo);
        /* Disabled/offline BLE overflow cannot release host Solo. */
        CHECK(smc_midi_host(host_pkt(0x9F,8,127))); smc_poll();
        smc_ble_link_slot(p,0); smc_poll();
        CHECK(smc_midi_host(host_pkt(0x9F,8,127))); smc_poll();
        fill(p); queue(p,0x80,8,0); smc_poll(); CHECK(smc_states[0].solo_target[0]);
        proj.set.bt &= (uint8_t)~PROJ_BT_SMC_CENTRAL;
        fill(p); queue(p,0x80,8,0); smc_poll(); CHECK(smc_states[0].solo_target[0]);
    }
    puts("PASS preset-at-ingress, exact frame length, foreign channels and offline overflow");
}
static void midpoll(void)
{
    for(unsigned p=0;p<2;p++) {
        prepare(1); inject_peer=p;
        queue(p,0x90,8,127); queue(p,0x80,9,0);
        inject_solo=1; smc_poll();
        CHECK(!inject_solo); CHECK(!smc_states[p].solo_target[1]);
        smc_poll(); CHECK(!smc_states[p].solo_target[0]); CHECK(smc_states[p].solo_target[1]);
        smc_poll(); CHECK(smc_states[p].solo_target[1]);
    }
    prepare(1); inject_peer=0;
    CHECK(smc_midi_host(host_pkt(0x9F,8,127)));
    CHECK(smc_midi_host(host_pkt(0x9F,10,127)));
    CHECK(smc_midi_host(host_pkt(0x8F,9,0)));
    inject_solo=2; smc_poll(); CHECK(!inject_solo);
    smc_poll(); CHECK(!smc_states[0].solo_target[0]);
    CHECK(!smc_states[0].solo_target[2]); CHECK(smc_states[0].solo_target[1]);
    for(unsigned p=0;p<2;p++) {
        prepare(1); inject_peer=p;
        queue(p,0x90,0x5D,127); queue(p,0x80,0x5D,0); smc_poll();
        CHECK(smc_states[p].locked);
        queue(p,0x90,0x5D,127); smc_poll(); now_ms+=900;
        queue(p,0x90,8,127); inject_solo=3; smc_poll();
        CHECK(smc_states[p].locked); /* a mid-poll edge cancels the old unlock hold */
        smc_poll(); CHECK(smc_states[p].locked && !smc_states[p].lock_down);
    }
    puts("PASS mid-poll BLE and host epochs preserve fresh presses and reject old host edges");
}
static void stop_fifo(void)
{
    for(unsigned src=0;src<2;src++) for(unsigned start=0;start<256;start++) {
        prepare(1); seq.req_stop=seq.req_start=seq.playing=0;
        smc_mmc_r[src]=smc_mmc_w[src]=(uint8_t)start;
        smc_mmc_stop_r[src]=smc_mmc_stop_w[src]=(uint8_t)start;
        for(unsigned i=0;i<16;i++) smc_mmc_host(1,src);
        smc_mmc_host(34,src); smc_mmc_host(34,src); /* same boundary coalesces */
        CHECK((uint8_t)(smc_mmc_stop_w[src]-smc_mmc_stop_r[src])==1);
        smc_mmc_r[src]++; /* consumer frees a harmless mute slot before ISR refill */
        smc_mmc_host(33,src); smc_mmc_host(34,src);
        CHECK((uint8_t)(smc_mmc_stop_w[src]-smc_mmc_stop_r[src])==2);
        smc_poll(); CHECK(seq.req_stop && !seq.req_start);
        CHECK(smc_mmc_r[src]==smc_mmc_w[src]);
        CHECK(smc_mmc_stop_r[src]==smc_mmc_stop_w[src]);
    }
    prepare(0); smc_mmc_host(33,0); proj.set.bt|=PROJ_BT_SMC_USER;
    smc_poll(); CHECK(!seq.req_start);
    prepare(1);
    for(unsigned src=0;src<2;src++) {
        for(unsigned i=0;i<16;i++) smc_mmc_host(1,src);
        smc_mmc_host(34,src);
    }
    smc_project_reset(); smc_poll(); CHECK(!seq.req_stop && !seq.req_start);
    puts("PASS repeated STOP FIFO, duplicate cutoff, both sources, all cursor rollovers and project reset");
}
static void feedback_epochs(void)
{
    for(unsigned p=0;p<2;p++) {
        prepare(0); smc_slot=(uint8_t)p;
        memset(smc.known,0,sizeof smc.known); smc.pending=0xFF;
        reject_tx=1; smc_frame_peer(); CHECK(!smc.known[0]); CHECK(smc.pending==0xFF);
        reject_tx=0; inject_peer=p; inject_feedback=1; smc_frame_peer();
        CHECK(!inject_feedback); CHECK(!smc.known[0]); CHECK(smc.pending==0xFF);
        smc_poll(); smc_frame_peer(); CHECK(smc.known[0]); CHECK(!smc.pending);
        memset(smc.known,1,sizeof smc.known); smc.pending=1;
        inject_feedback=1; smc_frame_peer(); CHECK(smc.pending==1);
        smc_poll(); smc_frame_peer(); CHECK(!smc.pending);
        smc_slot=0;
    }
    puts("PASS rejected feedback retries and link changes cannot acknowledge LEDs or faders");
}
int main(void)
{
    peer_loss_epochs(); ingress_and_foreign(); midpoll(); stop_fifo(); feedback_epochs();
    printf("SMC port production regression PASS: %d checks\n",checks);
    return 0;
}
