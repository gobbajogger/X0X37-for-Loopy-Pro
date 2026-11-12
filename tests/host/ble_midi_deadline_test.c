/* SPDX-License-Identifier: GPL-3.0-only */
/* HCI, mailbox, time and radio state are simulated. The service body is production code. */
#define main signal_test_main
#include "ble_midi_signal_test.c"
#undef main

static unsigned disconnect_attempts;
static int deadline_send(uint8_t t, const uint8_t *p, uint16_t n, void *ctx)
{
    if (t == 1 && n == 6 && p[0] == 6 && p[1] == 4) disconnect_attempts++;
    return send_h4(t, p, n, ctx);
}
static struct {
    struct { blec_t c,c2; } host;
    struct { unsigned state, polls, c2h; } mb;
    uint8_t fr[128];
} ble_r;
enum { XB_ST_READY = 1, BLE_GUARD_OK_MS = 1000 };
static unsigned ble_up, fm1_c1_on, ble_host_on, ble_want;
static uint32_t fm1_ms, ble_alive[2], ble_ok_at;
static struct { unsigned trying; } ble_guard;
static int smc_ble_role_get(void) { return 1; }
static void ble_record(void) { assert(0); }
static void ble_fail(void) { assert(0); }
static void ble_guard_ok(void *p) { (void)p; }
static void ble_central_link_notify(int up) { assert(!up); }
static void ble_central_link_notify_slot(unsigned slot,int up) { (void)slot; ble_central_link_notify(up); }
static int xb_frame_get(void *q, uint8_t *t, uint8_t *p, unsigned n)
{ (void)q; (void)t; (void)p; (void)n; return 0; }
static unsigned xb_ring_used(void *q) { (void)q; return 0; }
#include "ble_central_service.inc"

static void setup(blec_t *c, uint32_t start)
{
    ready(c); c->send = deadline_send; c->credits = 0;
    c->now_ms = now = start; disconnect_attempts = 0;
}
static void begin(blec_t *c, unsigned overflow, uint32_t start)
{
    setup(c, start); busy = 1;
    request(c, 1);
    if (overflow) for (unsigned id = 2; id <= 5; id++) request(c, (uint8_t)id);
    else blec_poll(c, start + 3000u);
    assert(c->state == BLEC_DISCONNECTING && !sent && disconnect_attempts == 1);
}
static void down(blec_t *c, unsigned status, unsigned handle)
{
    const uint8_t event[] = {5, 4, (uint8_t)status, (uint8_t)handle, 0, 0x13};
    blec_rx(c, 4, event, sizeof event, c->now_ms);
}
static void stopped(blec_t *c, uint32_t deadline, unsigned before)
{
    assert(c->state == BLEC_FAILED && !c->waiting && c->conn == 0x50 && !c->credits);
    assert(c->deadline == deadline && disconnect_attempts == before && !sent);
    blec_stop(c); blec_poll(c, deadline + 10000u);
    assert(c->state == BLEC_FAILED && c->deadline == deadline && disconnect_attempts == before);
    down(c, 0x0c, 0x50); down(c, 0, 0x51);
    assert(c->conn == 0x50 && !c->credits);
    down(c, 0, 0x50);
    assert(c->state == BLEC_OFF && !c->conn && c->credits == 5);
}
static void cutoff(unsigned overflow, uint32_t start, int offset, unsigned service)
{
    blec_t local, *c = service ? &ble_r.host.c : &local;
    begin(c, overflow, start);
    uint32_t deadline = c->deadline, tick = deadline + (uint32_t)offset;
    unsigned before = disconnect_attempts;
    if (service) {
        blec_poll(c, deadline - 2u); /* The stored time precedes the current service. */
        before = disconnect_attempts;
        ble_up = fm1_c1_on = ble_host_on = 1; ble_want = 0;
        ble_r.mb.state = XB_ST_READY; fm1_ms = tick;
        busy = 0; ble_central_service();
        assert(c->now_ms == tick);
    } else {
        const uint8_t ignored[] = {0xff, 0};
        blec_rx(c, 4, ignored, sizeof ignored, tick);
        busy = 0; blec_stop(c);
    }
    if (offset < 0) {
        assert(sent == 1 && disconnect_attempts == before + 1 && c->state == BLEC_STOPPING);
        assert(c->deadline == deadline && c->conn == 0x50 && !c->credits);
        blec_stop(c); blec_poll(c, tick);
        assert(sent == 1 && disconnect_attempts == before + 1);
        down(c, 0, 0x50); assert(c->state == BLEC_OFF && c->credits == 5);
    } else stopped(c, deadline, before);
}
static void cancel_cutoff(uint32_t start, int offset, unsigned enhanced)
{
    blec_t c; setup(&c, start); c.conn = 0; c.linked = 0; c.state = BLEC_CONNECTING;
    c.step = 8; c.waiting = 0;
    blec_stop(&c);
    uint32_t deadline = c.deadline;
    assert(c.state == BLEC_STOPPING && c.waiting && c.cmd_op == 0x200e);
    /* Count only Disconnect submissions after the accepted Cancel. */
    /* The controller completes Cancel first; that event returns the command
     * credit the per-link flow control (b26fb36e) needs for Disconnect. */
    const uint8_t cancel_done[] = {0x0e, 4, 1, 0x0e, 0x20, 0};
    blec_rx(&c, 4, cancel_done, sizeof cancel_done, deadline - 1u);
    sent = 0;
    const uint8_t event[] = {0x3e, 0x13, enhanced ? 0x0a : 1, 0, 0x50,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    blec_rx(&c, 4, event, sizeof event, deadline + (uint32_t)offset);
    if (offset < 0) {
        assert(disconnect_attempts == 1 && sent == 1 && c.state == BLEC_STOPPING);
        assert(c.deadline == deadline);
        down(&c, 0, 0x50); assert(c.state == BLEC_OFF && c.credits == 5);
    } else stopped(&c, deadline, 0);
}
static void normal_stop(uint32_t start)
{
    blec_t *c = &ble_r.host.c; setup(c, start);
    c->deadline = start - 100u; /* A ready link has no active abort interval. */
    ble_up = fm1_c1_on = ble_host_on = 1; ble_want = 0;
    ble_r.mb.state = XB_ST_READY; fm1_ms = start + 100u;
    ble_central_service();
    assert(c->state == BLEC_STOPPING && sent == 1 && disconnect_attempts == 1);
    assert(c->deadline == fm1_ms + 3000u && c->conn == 0x50 && !c->credits);
    ble_central_service(); assert(sent == 1);
}
int main(int argc, char **argv)
{
    for (unsigned wrap = 0; wrap <= 1; wrap++) {
        uint32_t start = wrap ? 0xfffffff0u : 100u;
        for (int offset = -1; offset <= 1; offset++) {
            for (unsigned overflow = 0; overflow <= 1; overflow++) {
                if (argc == 1 || !strcmp(argv[1], "stop")) cutoff(overflow, start, offset, 0);
                if (argc == 1 || !strcmp(argv[1], "service")) cutoff(overflow, start, offset, 1);
            }
            for (unsigned enhanced = 0; enhanced <= 1; enhanced++)
                if (argc == 1 || !strcmp(argv[1], "cancel")) cancel_cutoff(start, offset, enhanced);
        }
        if (argc == 1) normal_stop(start);
    }
    puts("Disconnect cutoff, production service time and cancel race: PASS");
    return 0;
}
