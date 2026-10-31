/* SPDX-License-Identifier: GPL-3.0-only */
/* Production central HCI handle-zero lifecycle; simulated transport, no device. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_central.h"

typedef struct { unsigned commands, acl_tx, messages; uint16_t op, handle; } trace_t;
static int tx(uint8_t type, const uint8_t *p, uint16_t n, void *ctx)
{
    trace_t *t = ctx;
    if (type == 1) {
        assert(n >= 3); t->commands++;
        t->op = (uint16_t)(p[0] | p[1] << 8);
        if (t->op == 0x0406) { assert(n == 6); t->handle = (uint16_t)(p[3] | p[4] << 8); }
    } else if (type == 2) {
        assert(n >= 8); t->acl_tx++; t->handle = (uint16_t)((p[0] | p[1] << 8) & 0x0fffu);
    } else assert(0);
    return 0;
}
static void rx_midi(const uint8_t *p, uint8_t n, void *ctx)
{
    trace_t *t = ctx;
    assert(n == 3 && p[0] == 0xb0 && p[1] == 16 && p[2] == 1);
    t->messages++;
}
static void init(blec_t *c, trace_t *t)
{
    static const uint8_t addr[6] = {0};
    memset(t, 0, sizeof *t);
    blec_init(c, tx, rx_midi, t, addr);
}
static void ready(blec_t *c, trace_t *t, unsigned handle)
{
    init(c, t);
    c->state = BLEC_CONNECTED; c->step = 14; c->conn = (uint16_t)handle;
    c->linked = 1; /* 0.0.22: linked marks any live handle, including 0 */
    c->credits = c->acl_total = 5; c->midi_handle = 114; c->cccd_handle = 115;
    c->now_ms = 100;
}
static void notify(blec_t *a, blec_t *b, unsigned handle, unsigned paired)
{
    uint8_t p[] = {0, 0x20, 12, 0, 8, 0, 4, 0, 0x1b, 114, 0, 0x80, 0x81, 0xb0, 16, 1};
    p[0] = (uint8_t)handle; p[1] = (uint8_t)((handle >> 8) | 0x20u);
    if (paired) blec_pair_rx(a, b, 2, p, sizeof p, 100);
    else blec_rx(a, 2, p, sizeof p, 100);
}
static void disconn(blec_t *a, blec_t *b, unsigned handle, unsigned paired)
{
    const uint8_t p[] = {0x05, 4, 0, (uint8_t)handle, (uint8_t)(handle >> 8), 0x13};
    if (paired) blec_pair_rx(a, b, 4, p, sizeof p, 100);
    else blec_rx(a, 4, p, sizeof p, 100);
}
static void single_handle_zero(void)
{
    blec_t a; trace_t ta;
    const uint8_t event[] = {0x3e, 19, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    blec_pair_bind(0, 0);
    init(&a, &ta);
    /* No connection: handle-zero Number of Completed Packets must not grant credits. */
    a.state = BLEC_CONNECTING; a.step = 8; a.credits = a.acl_total = 5;
    const uint8_t completed[] = {0x13, 5, 1, 0, 0, 9, 0};
    blec_rx(&a, 4, completed, sizeof completed, 100);
    assert(a.credits == 5);
    blec_rx(&a, 4, event, sizeof event, 100);
    assert(a.state == BLEC_DISCOVERING && a.conn == 0 && a.linked && a.att_waiting);
    assert(ta.acl_tx == 1 && ta.handle == 0); /* Service discovery on handle 0. */
    /* Stop must send a real HCI Disconnect for handle 0, not fall into Cancel. */
    blec_stop(&a);
    assert(a.state == BLEC_STOPPING && a.linked);
    assert(ta.commands && ta.op == 0x0406 && ta.handle == 0);
    disconn(&a, 0, 0, 0);
    assert(a.state == BLEC_OFF && !a.linked && !a.conn);
    blec_start(&a, 200);
    assert(a.state == BLEC_STARTING && !a.linked && !a.conn);
}
static __attribute__((unused)) void paired_handle_zero(void)
{
    blec_t a, b; trace_t ta, tb;
    ready(&a, &ta, 0); ready(&b, &tb, 0x50);
    blec_pair_bind(&a, &b);
    notify(&a, &b, 0, 1);
    assert(ta.messages == 1 && tb.messages == 0 && a.rx_acl == 1 && b.rx_acl == 0);
    notify(&a, &b, 0x50, 1);
    assert(ta.messages == 1 && tb.messages == 1 && a.rx_acl == 1 && b.rx_acl == 1);
    notify(&a, &b, 0x51, 1); /* Unknown handle must never reach either peer. */
    assert(ta.messages == 1 && tb.messages == 1 && a.rx_acl == 1 && b.rx_acl == 1);
    const uint8_t send[] = {0x90, 60, 127};
    assert(blec_midi_send(&a, send, sizeof send));
    blec_poll(&a, 100);
    assert(ta.acl_tx == 1 && ta.handle == 0);
    blec_stop(&a);
    assert(ta.op == 0x0406 && ta.handle == 0);
    assert(b.state == BLEC_CONNECTED && b.conn == 0x50);
    disconn(&a, &b, 0, 1);
    assert(a.state == BLEC_OFF && !a.linked && !a.conn);
    assert(b.state == BLEC_CONNECTED && b.conn == 0x50);
    notify(&a, &b, 0, 1);
    assert(ta.messages == 1 && tb.messages == 1);
    notify(&a, &b, 0x50, 1);
    assert(tb.messages == 2);
    disconn(&a, &b, 0x50, 1);
    assert(b.state == BLEC_OFF && !b.conn && !b.linked);
}
static void stopped_connection_race(void)
{
    blec_t a; trace_t ta;
    const uint8_t event[] = {0x3e, 19, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    blec_pair_bind(0, 0);
    init(&a, &ta);
    a.state = BLEC_CONNECTING; a.step = 8; a.credits = a.acl_total = 5; a.now_ms = 100;
    a.deadline = 5000; /* 0.0.22 stop cutoff: the connect attempt is still inside its deadline */
    blec_stop(&a);
    assert(a.state == BLEC_STOPPING && ta.commands == 1 && ta.op == 0x200e);
    blec_rx(&a, 4, event, sizeof event, 101);
    assert(a.conn == 0 && a.linked && a.state == BLEC_STOPPING);
    {   /* 0.0.22 HCI flow control: the cancel's Command Complete returns the credit. */
        const uint8_t cc[] = {0x0e, 4, 1, 0x0e, 0x20, 0};
        blec_rx(&a, 4, cc, sizeof cc, 102);
        for (int i = 0; i < 3 && ta.commands < 2; i++) blec_poll(&a, 103u + (unsigned)i);
    }
    assert(ta.commands == 2 && ta.op == 0x0406 && ta.handle == 0);
    disconn(&a, 0, 0, 0);
    assert(a.state == BLEC_OFF && !a.linked);
}
static __attribute__((unused)) void no_phantom_zero_peer(void)
{
    blec_t a, b; trace_t ta, tb;
    ready(&a, &ta, 0); a.linked = 0; /* Zero in an unowned slot. */
    ready(&b, &tb, 0x50); blec_pair_bind(&a, &b);
    notify(&a, &b, 0, 1);
    assert(!a.rx_acl && !b.rx_acl && !ta.messages && !tb.messages);
    const uint8_t p[] = {0x13, 5, 1, 0, 0, 255, 0};
    blec_pair_rx(&a, &b, 4, p, sizeof p, 100);
    assert(a.credits == 5 && b.credits == 5); /* No phantom completion on handle zero. */
}
int main(void)
{
    single_handle_zero();
#if BLEC_TWO_PEER
    paired_handle_zero(); no_phantom_zero_peer(); /* two LE links only */
#endif
    stopped_connection_race();
    puts("BLE handle 0: connect, ATT, STOP, disconnect, pair routing and no phantom: PASS");
    return 0;
}
