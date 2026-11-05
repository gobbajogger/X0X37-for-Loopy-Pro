/* SPDX-License-Identifier: GPL-3.0-only */
/* Public HCI and actual service tests. Mailbox, peer and time are simulated. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_central.h"
static struct {
    struct { blec_t c,c2; } host;
    struct { unsigned state, polls, c2h; } mb;
    uint8_t fr[128];
} ble_r;
enum { XB_ST_READY = 1, BLE_GUARD_OK_MS = 1000 };
static unsigned ble_up, fm1_c1_on, ble_host_on, ble_want, busy;
static uint32_t fm1_ms, ble_alive[2], ble_ok_at;
static struct { unsigned trying; } ble_guard;
static unsigned attempts[4], accepted[4], pending_n, linked;
static uint8_t pending[40];
static int smc_ble_role_get(void) { return 1; }
static void ble_record(void) { assert(0); }
static void ble_fail(void) { assert(0); }
static void ble_guard_ok(void *p) { (void)p; }
static void ble_central_link_notify(int up) { linked = (unsigned)up; }
static void ble_central_link_notify_slot(unsigned slot,int up) { (void)slot; ble_central_link_notify(up); }
static int xb_frame_get(void *q, uint8_t *t, uint8_t *p, unsigned n)
{
    (void)q;
    if (!pending_n) return 0;
    assert(pending_n <= n); unsigned have = pending_n;
    memcpy(p, pending, have); pending_n = 0; *t = 4;
    return (int)have;
}
static unsigned xb_ring_used(void *q) { (void)q; return pending_n; }
static int transport(uint8_t h4, const uint8_t *p, uint16_t n, void *ctx)
{
    (void)ctx;
    if (h4 == 2) return busy ? -1 : 0;
    assert(h4 == 1 && n >= 3);
    unsigned op = p[0] | (unsigned)p[1] << 8;
    unsigned index = op == 0x200e ? 0 : op == 0x200c ? 1 : op == 0x0406 ? 2 : 3;
    attempts[index]++;
    if (busy) return -1;
    accepted[index]++;
    return 0;
}
#include "ble_central_service.inc"
static void event(const uint8_t *p, unsigned n)
{ blec_rx(&ble_r.host.c, 4, p, (uint16_t)n, fm1_ms); }
static void response(unsigned op, unsigned status, unsigned complete)
{
    const uint8_t cc[] = {0x0e, 4, 1, (uint8_t)op, (uint8_t)(op >> 8), (uint8_t)status};
    const uint8_t cs[] = {0x0f, 4, (uint8_t)status, 1, (uint8_t)op, (uint8_t)(op >> 8)};
    event(complete ? cc : cs, 6);
}
static void connection(unsigned enhanced, unsigned status, unsigned service)
{
    uint8_t p[33] = {0x3e, enhanced ? 31 : 19, enhanced ? 0x0a : 1,
        (uint8_t)status, 0x50, 0, 0};
    unsigned n = enhanced ? 33 : 21;
    if (service) { memcpy(pending, p, n); pending_n = n; ble_central_service(); }
    else event(p, n);
}
static void down(unsigned status, unsigned handle)
{
    const uint8_t p[] = {5, 4, (uint8_t)status, (uint8_t)handle, 0, 0x13};
    event(p, sizeof p);
}
static void setup(unsigned create, uint32_t at)
{
    blec_t *c = &ble_r.host.c; const uint8_t addr[6] = {0};
    memset(&ble_r, 0, sizeof ble_r); memset(attempts, 0, sizeof attempts);
    memset(accepted, 0, sizeof accepted); pending_n = busy = linked = 0;
    ble_up = fm1_c1_on = ble_host_on = 1; ble_want = 1;
    ble_r.mb.state = XB_ST_READY; fm1_ms = at;
    blec_pair_bind(0, 0); /* Retain legacy single-peer edge cases; dual lifecycle has its own suite. */
    blec_init(c, transport, 0, 0, addr); blec_start(c, at);
    response(0x0c03, 0, 1); response(0x0c01, 0, 1); response(0x2001, 0, 1);
    const uint8_t buf[] = {0x0e, 7, 1, 2, 0x20, 0, 27, 0, 5}; event(buf, sizeof buf);
    response(0x2005, 0, 1); response(0x200b, 0, 1); response(0x200c, 0, 1);
    assert(c->state == BLEC_SCANNING && !c->waiting);
    if (create) {
        const uint8_t adv[] = {0x3e, 17, 2, 1, 0, 1, 1, 2, 3, 4, 5, 6, 5, 4, 9, 'S', 'M', 'C', 0};
        event(adv, sizeof adv); response(0x200c, 0, 1);
        assert(c->state == BLEC_CONNECTING && c->waiting && c->cmd_op == 0x200d);
        response(0x200d, 0, 0); assert(!c->waiting);
    }
    memset(attempts, 0, sizeof attempts); memset(accepted, 0, sizeof accepted);
}
static uint32_t stop(unsigned refused)
{
    busy = refused; ble_want = 0; ble_central_service();
    assert(ble_r.host.c.state == BLEC_STOPPING && !linked);
    return ble_r.host.c.deadline;
}
static void retry(unsigned create, unsigned enhanced, uint32_t at)
{
    blec_t *c = &ble_r.host.c; setup(create, at);
    uint32_t cutoff = stop(1); unsigned index = create ? 0 : 1;
    assert(!accepted[index] && !c->waiting);
    blec_start(c, fm1_ms); assert(c->state == BLEC_STOPPING && c->deadline == cutoff);
    fm1_ms++; ble_central_service(); blec_stop(c); assert(c->deadline == cutoff);
    busy = 0; fm1_ms++; ble_central_service();
    assert(accepted[index] == 1 && c->waiting && c->state == BLEC_STOPPING);
    unsigned count = attempts[index];
    response(create ? 0x200e : 0x200c, 0, 0);
    assert(c->state == BLEC_STOPPING); /* Status alone cannot prove completion. */
    ble_central_service(); assert(attempts[index] == count);
    response(create ? 0x200e : 0x200c, 0, 1);
    if (create) {
        assert(c->state == BLEC_STOPPING && !c->waiting);
        ble_central_service(); assert(attempts[index] == count);
        c->credits = 0; connection(enhanced, 0, 1);
        assert(c->conn == 0x50 && accepted[2] == 1 && !c->credits && c->deadline == cutoff);
        connection(enhanced, 0, 1); assert(accepted[2] == 1);
        response(0x0406, 0, 0); ble_central_service(); assert(accepted[2] == 1);
        down(0x0c, 0x50); down(0, 0x51); assert(c->conn == 0x50 && !c->credits);
        down(0, 0x50); assert(c->credits == 5);
    }
    assert(c->state == BLEC_OFF && !c->conn && !linked);
    ble_want = 1; fm1_ms++; ble_central_service(); assert(c->state == BLEC_STARTING);
}
static void expiry(unsigned create, unsigned enhanced, uint32_t at, unsigned accept, int offset)
{
    blec_t *c = &ble_r.host.c; setup(create, at); uint32_t cutoff = stop(!accept);
    if (accept) response(create ? 0x200e : 0x200c, 0, 1);
    fm1_ms = cutoff; ble_central_service();
    if (!create && accept) { assert(c->state == BLEC_OFF); return; }
    assert(c->state == BLEC_FAILED && !c->waiting && c->deadline == cutoff);
    unsigned before[4]; memcpy(before, attempts, sizeof before);
    blec_stop(c); blec_start(c, fm1_ms); busy = 0; fm1_ms++; ble_central_service();
    assert(c->state == BLEC_FAILED && c->deadline == cutoff && !memcmp(before, attempts, sizeof before));
    if (create) {
        c->credits = 0; fm1_ms = cutoff + (uint32_t)offset; connection(enhanced, 0, 1);
        assert(c->conn == 0x50 && !c->credits && !accepted[2] && c->state == BLEC_FAILED);
        down(0x0c, 0x50); down(0, 0x51); assert(c->conn == 0x50 && !c->credits);
        down(0, 0x50); assert(c->state == BLEC_OFF && c->credits == 5);
    }
}
static void failure(unsigned create, unsigned complete)
{
    blec_t *c = &ble_r.host.c; setup(create, 100); uint32_t cutoff = stop(0);
    response(create ? 0x200e : 0x200c, 0x0c, complete);
    assert(c->state == BLEC_FAILED && !c->waiting && c->deadline == cutoff);
    unsigned count = attempts[create ? 0 : 1];
    blec_stop(c); ble_central_service(); assert(attempts[create ? 0 : 1] == count);
    if (create) {
        connection(1, 0, 1); assert(c->conn == 0x50 && accepted[2] == 1 && c->deadline == cutoff);
        down(0, 0x50); assert(c->state == BLEC_OFF);
    }
}
static void inflight(void)
{
    blec_t *c = &ble_r.host.c;
    for (unsigned create = 0; create <= 1; create++) {
        setup(create, 100);
        c->waiting = 1; c->cmd_op = create ? 0x200d : 0x200c;
        uint32_t cutoff = stop(0); assert(!accepted[create ? 0 : 1]);
        response(c->cmd_op, 0, 0);
        if (!create) {
            assert(c->waiting); ble_central_service(); assert(!accepted[1]);
            response(0x200c, 0, 1);
        }
        assert(c->state == BLEC_STOPPING && accepted[create ? 0 : 1] == 1 && c->deadline == cutoff);
    }
    setup(1, 100); busy = 1; uint32_t cutoff = stop(1);
    fm1_ms = cutoff - 1; busy = 0; connection(0, 0, 1);
    assert(c->conn == 0x50 && accepted[2] == 1 && c->deadline == cutoff);
    setup(1, 100); stop(0); connection(0, 2, 1);
    assert(c->state == BLEC_OFF && !c->conn);
    setup(0, 100); stop(1); connection(0, 2, 1);
    assert(c->state == BLEC_STOPPING); /* Unrelated failed connection is not scan-disable evidence. */
    setup(1, 100); c->waiting = 1; c->cmd_op = 0x200d;
    stop(0); response(0x200d, 0x0c, 0);
    assert(c->state == BLEC_OFF && !accepted[0]);
    setup(0, 100); c->step = 7; c->waiting = 1; c->cmd_op = 0x200c;
    stop(0); response(0x200c, 0, 1);
    assert(c->state == BLEC_OFF && !accepted[1]);
    setup(1, 100); c->step = 9; c->cmd_op = 0x200e; c->waiting = 1;
    c->deadline = 200; stop(0); response(0x200e, 0, 1); ble_central_service();
    assert(!accepted[0] && c->state == BLEC_STOPPING && c->deadline == 200);
}
int main(int argc, char **argv)
{
    for (unsigned wrap = 0; wrap <= 1; wrap++) {
        uint32_t at = wrap ? 0xfffffff0u : 100;
        if (argc == 1 || !strcmp(argv[1], "cancel"))
            for (unsigned enhanced = 0; enhanced <= 1; enhanced++) retry(1, enhanced, at);
        if (argc == 1 || !strcmp(argv[1], "scan")) retry(0, 0, at);
        if (argc == 1 || !strcmp(argv[1], "edges"))
            for (unsigned create = 0; create <= 1; create++)
                for (unsigned accept = 0; accept <= 1; accept++)
                    for (unsigned enhanced = 0; enhanced <= 1; enhanced++)
                        for (int offset = 0; offset <= 1; offset++) expiry(create, enhanced, at, accept, offset);
    }
    if (argc == 1 || !strcmp(argv[1], "failed-status"))
        for (unsigned create = 0; create <= 1; create++)
            for (unsigned complete = 0; complete <= 1; complete++) failure(create, complete);
    if (argc == 1 || !strcmp(argv[1], "edges")) inflight();
    puts("Stop acceptance, status, original cutoff, late events and production service: PASS");
    return 0;
}
