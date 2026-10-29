/* SPDX-License-Identifier: GPL-3.0-only */
/* Real host state machines and mailbox rings; only platform/UI glue is simulated. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "x0x_ble_abi.h"
#include "ble_midi_host.c"
#include "ble_midi_central.c"
#include "ble_glue.h"
static struct {
    xb_mbox_t mb;
    union { bleh_t h; struct { blec_t c, c2; }; } host;
    uint8_t fr[XB_FRAME_MAX];
    char nm[9];
    unsigned ow, orr;
} ble_r;
static uint8_t ble_central_role, ble_up, ble_host_on, ble_want, desired;
static uint8_t ble_sx_n, ble_sx_over, ble_sx_reply, ble_sx_ch, ble_link_st;
static uint32_t fm1_ms;
static unsigned failures, releases, queue_full, apc_up, central_up, reset_count;
static int smc_ble_role_get(void) { return desired; }
static void ble_fail(void) { failures++; ble_up = 0; }
static void ble_central_link_notify(int up) { central_up = (unsigned)up; }
static void ble_central_link_notify_slot(unsigned slot, int up) { if (!slot) central_up = (unsigned)up; }
static void smk_apc_ble_link(int up, int notify) { apc_up = (unsigned)(up && notify); }
static int ble_send(uint8_t type, const uint8_t *p, uint16_t n, void *ctx)
{
    (void)ctx;
    if (type == 1 && rd16(p) == OP_RESET) reset_count++;
    return xb_frame_put(&ble_r.mb.h2c, type, p, n);
}
static void ble_central_midi(const uint8_t *p, uint8_t n, void *ctx)
{ (void)p; (void)n; (void)ctx; }
static void ble_sysex_in(const uint8_t *p, uint32_t n, int end, void *ctx)
{ (void)p; (void)n; (void)end; (void)ctx; }
/* Platform surfaces around the production TIMER5 service bodies. */
enum { MQ = 64, BLE_OQ = 64 };
static uint32_t mi_w, mi_r, midi_in_q[MQ], ble_oq[BLE_OQ];
static int midi_in_put(uint32_t pkt)
{
    if (mi_w - mi_r >= MQ) return 0;
    midi_in_q[mi_w++ % MQ] = pkt;
    return 1;
}
static uint32_t ble_alive[2], ble_ok_at;
static unsigned fm1_c1_on = 1;
static ble_guard_t ble_guard;
#define RING_PUBLISH() ((void)0)
static void ble_record(void) { assert(0); }
static void smk_inlog(unsigned src, uint32_t data) { (void)src; (void)data; }
static unsigned smk_apc_ble_event(uint32_t p, uint8_t notify) { (void)p; (void)notify; return 0; }
static void ble_sx_answer(bleh_t *h) { (void)h; }
static void midi_link_check(uint8_t *st, uint32_t up, uint32_t src)
{
    (void)src;
    uint8_t v = (uint8_t)((*st & 2u) | ((*st & 1u) && !up ? 2u : 0u));
    if ((v & 2u) && !queue_full) { releases++; v = 0; }
    *st = (uint8_t)(v | (up ? 1u : 0u));
}
#include "ble_role_services.inc"
#include "ble_role.h"
static const uint8_t address[6] = {1, 2, 3, 4, 5, 0xc6};
static void setup(unsigned central)
{
    memset(&ble_r, 0, sizeof ble_r);
    strcpy(ble_r.nm, "X0X-TEST");
    xb_ring_init(&ble_r.mb.h2c); xb_ring_init(&ble_r.mb.c2h);
    ble_r.mb.state = XB_ST_READY;
    mi_w = mi_r = 0;
    ble_central_role = desired = (uint8_t)central;
    ble_up = ble_host_on = ble_want = 1;
    ble_role_changing = ble_link_st = 0; ble_role_at = 0;
    failures = releases = queue_full = reset_count = apc_up = central_up = 0;
    fm1_ms = 10;
    ble_r.ow = 9; ble_r.orr = 2;
    ble_sx_n = ble_sx_over = ble_sx_reply = ble_sx_ch = 1;
    if (central) {
        blec_init(&ble_r.host.c, ble_send, ble_central_midi, 0, address);
        blec_init(&ble_r.host.c2, ble_send, ble_central_midi, (void *)1, address);
        blec_pair_bind(&ble_r.host.c, &ble_r.host.c2);
    } else { blec_pair_bind(0, 0); bleh_init(&ble_r.host.h, ble_send, 0, ble_r.nm, address); }
}
static unsigned take_command(void)
{
    uint8_t p[256], t;
    int n = xb_frame_get(&ble_r.mb.h2c, &t, p, sizeof p);
    assert(n >= 3 && t == 1);
    return rd16(p);
}
static void complete(unsigned op)
{
    uint8_t p[] = {0x0e, 4, 1, (uint8_t)op, (uint8_t)(op >> 8), 0};
    assert(xb_frame_put(&ble_r.mb.c2h, 4, p, sizeof p) == 0);
}
static void disconnected(void)
{
    uint8_t p[] = {5, 4, 0, 0x40, 0, 0x13};
    assert(xb_frame_put(&ble_r.mb.c2h, 4, p, sizeof p) == 0);
}
static void assert_clean(unsigned role)
{
    assert(ble_central_role == role && !ble_host_on && !ble_role_changing);
    assert(ble_r.orr == ble_r.ow && !ble_sx_n && !ble_sx_over && !ble_sx_reply && !ble_sx_ch);
    assert(!apc_up && !central_up && !failures);
    assert(!memcmp(role ? ble_r.host.c.addr : ble_r.host.h.addr, address, 6));
    if (!role) assert(!strcmp(ble_r.host.h.name, "X0X-TEST") && ble_r.host.h.sysex == ble_sysex_in);
}
static void device_to_host(void)
{
    setup(0); bleh_t *h = &ble_r.host.h;
    h->state = BLEH_CONNECTED; h->conn = 0x40; h->cccd[0] = 1;
    ble_link_st = 1; queue_full = 1; apc_up = 1; desired = 1;
    ble_service(); assert(ble_role_changing && !ble_central_role && ble_link_st == 2);
    assert(take_command() == OP_DISC && !reset_count);
    complete(OP_DISC); disconnected(); ble_service();
    assert(!ble_central_role && ble_link_st == 2 && !reset_count);
    queue_full = 0; ble_service(); assert(releases == 1); assert_clean(1);
    ble_service(); assert(take_command() == OP_RESET && reset_count == 1);
    assert(ble_r.host.c.state == BLEC_STARTING);
}
static void host_to_device(void)
{
    setup(1); blec_t *c = &ble_r.host.c;
    c->state = BLEC_CONNECTED; c->conn = 0x40;c->linked=1; c->step = S_READY;
    central_up = 1; desired = 0;
    ble_service(); assert(ble_central_role && ble_role_changing);
    assert(take_command() == COP_DISCONNECT && !reset_count);
    /* Command acceptance does not prove link teardown. */
    uint8_t status[] = {0x0f, 4, 0, 1, 6, 4};
    assert(xb_frame_put(&ble_r.mb.c2h, 4, status, sizeof status) == 0);
    ble_service(); assert(ble_central_role && !reset_count);
    disconnected(); ble_service(); assert_clean(0);
    ble_service(); assert(take_command() == OP_RESET && reset_count == 1);
}
static void pending_and_reversal(void)
{
    setup(0); bleh_t *h = &ble_r.host.h;
    h->state = BLEH_ADVERTISING; h->want_adv = 1; h->cmd_wait = 1; h->cmd_op = OP_ADV_EN;
    desired = 1; ble_service(); assert(!ble_central_role && !xb_ring_used(&ble_r.mb.h2c));
    complete(OP_ADV_EN); ble_service(); assert(take_command() == OP_ADV_EN && !ble_central_role);
    complete(OP_ADV_EN); ble_service(); assert_clean(1);
    setup(1); blec_t *c = &ble_r.host.c;
    c->state = BLEC_SCANNING; c->step = S_SCAN_ON;
    desired = 0; ble_service(); assert(take_command() == COP_SCAN_ENABLE);
    desired = 1; ble_service(); assert(ble_central_role && !ble_role_changing && !reset_count);
    complete(COP_SCAN_ENABLE); ble_service();
    assert(ble_central_role && reset_count == 1 && take_command() == COP_RESET);
}
static void off_and_timeout(void)
{
    setup(0); ble_want = ble_host_on = 0; desired = 1;
    ble_service(); assert_clean(1); ble_service(); assert(!reset_count);
    desired = 0; ble_service(); assert_clean(0); ble_service(); assert(!reset_count);
    setup(0); ble_r.host.h.state = BLEH_ADVERTISING;
    ble_r.host.h.cmd_wait = 1; desired = 1;
    fm1_ms = UINT32_MAX - 1000u; ble_service();
    fm1_ms += 2999u; ble_service(); assert(!failures && !ble_central_role);
    fm1_ms++; ble_service(); assert(failures == 1 && !ble_up && !ble_central_role && !reset_count);
}
static void connecting_cancel_race(void)
{
    setup(1); blec_t *c = &ble_r.host.c;
    c->state = BLEC_CONNECTING; c->step = S_CREATE; c->cmd_op = COP_CREATE;
    desired = 0; ble_service(); assert(take_command() == COP_CANCEL);
    complete(COP_CANCEL); ble_service(); assert(ble_central_role && c->state == BLEC_STOPPING);
    /* A successful connection after Cancel must be disconnected before the union changes. */
    uint8_t connection[21] = {0x3e, 19, 1, 0, 0x40, 0, 0};
    assert(xb_frame_put(&ble_r.mb.c2h, 4, connection, sizeof connection) == 0);
    ble_service(); assert(take_command() == COP_DISCONNECT && ble_central_role);
    complete(COP_DISCONNECT); disconnected(); ble_service(); assert_clean(0);
}
static void startup_and_backlog(void)
{
    setup(0); bleh_start(&ble_r.host.h);
    assert(take_command() == OP_RESET); desired = 1;
    ble_service(); assert(!ble_central_role && ble_r.host.h.state == BLEH_STARTING);
    const unsigned commands[] = {OP_RESET, OP_EVMASK, OP_LE_EVMASK, OP_LE_BUF,
                                 OP_RAND, OP_ADV_PARAM, OP_ADV_DATA, OP_SCAN_RSP};
    for (unsigned i = 0; i < sizeof commands / sizeof commands[0]; i++) {
        complete(commands[i]); ble_service();
        if (i + 1 < sizeof commands / sizeof commands[0])
            assert(take_command() == commands[i + 1] && !ble_central_role);
    }
    assert_clean(1);
    setup(0); desired = 1;
    /* A burst larger than a service tick must drain in the old parser. */
    for (unsigned i = 0; i < 9; i++) complete(OP_RESET);
    ble_service(); assert(!ble_central_role && xb_ring_used(&ble_r.mb.c2h));
    ble_service(); assert_clean(1);
    setup(0); desired = 1;
    uint8_t command[] = {1, 0, 0};
    assert(xb_frame_put(&ble_r.mb.h2c, 1, command, sizeof command) == 0);
    ble_service(); assert(!ble_central_role && !reset_count);
    assert(take_command() == 1); ble_service(); assert_clean(1);
}
static void failed_stop(void)
{
    setup(1); blec_t *c = &ble_r.host.c;
    c->state = BLEC_CONNECTED; c->conn = 0x40;c->linked=1; c->step = S_READY; desired = 0;
    ble_service(); assert(take_command() == COP_DISCONNECT);
    uint8_t error[] = {0x0f, 4, 0x0c, 1, 6, 4};
    assert(xb_frame_put(&ble_r.mb.c2h, 4, error, sizeof error) == 0);
    ble_service(); assert(ble_central_role && c->conn == 0x40 && c->state == BLEC_FAILED);
    fm1_ms += 3000; ble_service();
    assert(failures == 1 && !ble_up && ble_central_role && c->conn == 0x40 && !reset_count);
}
static void two_host_links_stop(void)
{
    setup(1); blec_t *a=&ble_r.host.c,*b=&ble_r.host.c2;
    a->state=b->state=BLEC_CONNECTED;a->step=b->step=S_READY;a->conn=0x40;a->linked=1;b->conn=0x41;b->linked=1;
    desired=0;ble_service();assert(take_command()==COP_DISCONNECT);
    complete(COP_DISCONNECT);ble_service();assert(take_command()==COP_DISCONNECT);
    disconnected();ble_service();assert(ble_central_role && b->conn==0x41);
    /* Link down does not authorize changing the union while command status is outstanding. */
    uint8_t event[]={5,4,0,0x41,0,0x13};
    assert(xb_frame_put(&ble_r.mb.c2h,4,event,sizeof event)==0);
    ble_service();assert(ble_central_role && blec_command_owner);
    uint8_t no_credit[]={0x0e,4,0,6,4,0};
    assert(xb_frame_put(&ble_r.mb.c2h,4,no_credit,sizeof no_credit)==0);
    ble_service();assert(ble_central_role && ble_host_on && !reset_count);
    uint8_t credit[]={0x0e,4,1,0,0,0};
    assert(xb_frame_put(&ble_r.mb.c2h,4,credit,sizeof credit)==0);
    ble_service();assert_clean(0);
}
int main(void)
{
    two_host_links_stop(); device_to_host(); host_to_device(); pending_and_reversal(); off_and_timeout();
    connecting_cancel_race(); startup_and_backlog(); failed_stop();
    puts("PASS Bluetooth runtime role transitions, stop acknowledgement, release backpressure, reversal, OFF and timeout");
}
