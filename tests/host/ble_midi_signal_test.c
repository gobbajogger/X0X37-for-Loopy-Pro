/* SPDX-License-Identifier: GPL-3.0-only */
/* Public HCI tests. All packets are simulated; this is not a device test. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/ble/ble_midi_central.h"

static unsigned sent, messages, busy, now;
static uint8_t last[40], last_type, cmd[24];
static unsigned last_n, cmd_n;
static uint8_t reply_ids[16];
static unsigned replies;
static int send_h4(uint8_t t, const uint8_t *p, uint16_t n, void *ctx)
{
    (void)ctx;
    if (busy) return -1;
    assert(n <= sizeof last);
    memcpy(last, p, n); last_n = n; last_type = t; sent++;
    if (t == 1 && n <= sizeof cmd) { memcpy(cmd, p, n); cmd_n = n; }
    if (t == 2 && n >= 14 && p[6] == 5) {
        assert(replies < sizeof reply_ids); reply_ids[replies++] = p[9];
    }
    return 0;
}
static void midi(const uint8_t *p, uint8_t n, void *ctx)
{
    (void)ctx;
    assert(n == 3 && p[0] == 0xb0 && p[1] == 16 && p[2] == 1);
    messages++;
}
static void ready(blec_t *c)
{
    static const uint8_t addr[6] = {0};
    blec_init(c, send_h4, midi, 0, addr);
    c->state = BLEC_CONNECTED; c->step = 14; c->conn = 0x50;c->linked=1;
    c->midi_handle = 114; c->cccd_handle = 115;
    c->acl_total = c->credits = 5;
    sent = messages = busy = replies = cmd_n = 0; now = 100;
}
static void acl(blec_t *c, unsigned pb, unsigned cid, const uint8_t *p,
                unsigned n, unsigned total)
{
    uint8_t b[96] = {0x50, (uint8_t)(pb << 4), 0, 0};
    unsigned head = pb == 1 ? 4 : 8;
    assert(head + n <= sizeof b);
    b[2] = (uint8_t)(n + head - 4);
    if (head == 8) { b[4] = (uint8_t)total; b[5] = (uint8_t)(total >> 8); b[6] = (uint8_t)cid; }
    memcpy(b + head, p, n);
    blec_rx(c, 2, b, (uint16_t)(n + head), now);
}
static void signal(blec_t *c, const uint8_t *p, unsigned n)
{ acl(c, 2, 5, p, n, n); }
static void request(blec_t *c, uint8_t id)
{
    const uint8_t p[] = {0x12, id, 8, 0, 6, 0, 12, 0, 0, 0, 0x90, 1};
    signal(c, p, sizeof p);
}
static void reply(unsigned code, unsigned id, unsigned reason, unsigned mtu)
{
    const uint8_t want[] = {0x50, 0, mtu ? 12 : 10, 0, mtu ? 8 : 6, 0, 5, 0,
                           (uint8_t)code, (uint8_t)id, mtu ? 4 : 2, 0,
                           (uint8_t)reason, 0, (uint8_t)mtu, 0};
    assert(last_type == 2 && last_n == (mtu ? 16 : 14));
    assert(!memcmp(last, want, last_n));
}
static void done(blec_t *c, unsigned handle, unsigned count)
{
    const uint8_t e[] = {0x13, 5, 1, (uint8_t)handle, 0, (uint8_t)count, (uint8_t)(count >> 8)};
    blec_rx(c, 4, e, sizeof e, now);
}
static void notification(blec_t *c)
{
    const uint8_t p[] = {0x1b, 114, 0, 0x80, 0x81, 0xb0, 16, 1};
    unsigned before = messages;
    acl(c, 2, 4, p, sizeof p, sizeof p);
    assert(messages == before + 1);
}
static void lifecycle(void)
{
    blec_t c;
    const uint8_t down[] = {5, 4, 0, 0x50, 0, 0x13};
    const uint8_t failed_down[] = {5, 4, 0x0c, 0x50, 0, 0};
    ready(&c); c.credits = 0;
    blec_rx(&c, 4, down, sizeof down, now);
    assert(c.state == BLEC_SCANNING && c.credits == 5);
    /* The next connection must issue discovery without old completion events. */
    const uint8_t conn[] = {0x3e, 0x13, 1, 0, 0x51, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    c.state = BLEC_CONNECTING; c.step = 8; c.waiting = 0;
    blec_rx(&c, 4, conn, sizeof conn, now);
    assert(c.state == BLEC_DISCOVERING && c.att_waiting && c.credits == 4);
    assert(last_type == 2 && last[0] == 0x51 && last[6] == 4 && last[8] == 0x10);
    ready(&c); c.credits = 0;
    blec_rx(&c, 4, failed_down, sizeof failed_down, now);
    assert(blec_connected(&c) && !c.credits);
    ready(&c); c.credits = 1; done(&c, 0x50, 255);
    assert(c.credits == 5); /* Saturate before the uint8_t conversion. */
    puts("Reconnect credits: PASS");
}
int main(int argc, char **argv)
{
    blec_t c;
    if (argc > 1 && !strcmp(argv[1], "lifecycle")) { lifecycle(); return 0; }
    ready(&c); request(&c, 0xa7);
    assert(sent == 2 && cmd_n == 17 && cmd[0] == 0x13 && cmd[1] == 0x20 && cmd[2] == 14);
    assert(cmd[3] == 0x50 && cmd[4] == 0 && cmd[5] == 6 && cmd[6] == 0);
    assert(cmd[7] == 12 && cmd[8] == 0 && cmd[11] == 0x90 && cmd[12] == 1);
    reply(0x13, 0xa7, 0, 0);
    assert(c.credits == 4 && blec_connected(&c)); notification(&c);
    /* Every split includes a short final fragment and preserves the identifier. */
    for (unsigned split = 0; split < 12; split++) {
        uint8_t p[] = {0x12, 0xfe, 8, 0, 6, 0, 12, 0, 0, 0, 0x90, 1};
        ready(&c); acl(&c, 2, 5, p, split, sizeof p);
        assert(!sent); acl(&c, 1, 0, p + split, sizeof p - split, 0);
        assert(sent == 2 && cmd[0] == 0x13 && cmd[1] == 0x20);
        reply(0x13, 0xfe, 0, 0); notification(&c);
    }
    /* An illegal update is still refused. An in-range update was accepted above. */
    ready(&c);
    uint8_t p[80] = {0x12, 1, 8, 0}; signal(&c, p, 12);
    assert(sent == 1); reply(0x13, 1, 1, 0);
    p[0] = 0x08; p[1] = 2; p[2] = 0; signal(&c, p, 4);
    assert(sent == 2); reply(1, 2, 0, 0); /* BR/EDR echo is disallowed on CID 5. */
    p[0] = 0xff; p[1] = 3; signal(&c, p, 4);
    assert(sent == 3); reply(1, 3, 0, 0);
    /* Unsolicited recognized responses and invalid identifiers have no reply. */
    const uint8_t responses[] = {1, 7, 0x13, 0x15, 0x18, 0x1a};
    for (unsigned i = 0; i < sizeof responses; i++) { p[0] = responses[i]; signal(&c, p, 6); }
    assert(sent == 3);
    p[0] = 0x12; p[1] = 0; p[2] = 8; signal(&c, p, 12);
    p[1] = 4; signal(&c, p, 11); /* Truncated command. */
    p[2] = 7; signal(&c, p, 11); /* Wrong length for this command. */
    p[2] = 8; signal(&c, p, 16); /* Multiple commands are invalid on LE signaling. */
    signal(&c, p, 3); assert(sent == 3); notification(&c);
    /* Bounds: accept 64 bytes, report MTU for 65 and 65535, retain only 64 bytes. */
    for (unsigned total = 64; total <= 65535; total = total == 64 ? 65 : total == 65 ? 65535 : 65536) {
        ready(&c); memset(p, 0, sizeof p); p[0] = 0xff; p[1] = 0x99;
        p[2] = (uint8_t)(total - 4); p[3] = (uint8_t)((total - 4) >> 8);
        acl(&c, 2, 5, p, 2, total);
        /* Continue in bounded chunks, including the rest of the command header. */
        for (unsigned off = 2; off < total;) {
            unsigned n = total - off > 64 ? 64 : total - off;
            uint8_t chunk[64] = {0};
            if (off == 2) { chunk[0] = p[2]; chunk[1] = p[3]; }
            acl(&c, 1, 0, chunk, n, 0); off += n;
        }
        assert(sent == 1 && !c.rx_have && !c.rx_need);
        reply(1, 0x99, total > 64 ? 1 : 0, total > 64 ? 64 : 0); notification(&c);
    }
    ready(&c); memset(p, 0, sizeof p); p[0] = 0xff; p[1] = 0x80; p[2] = 76;
    signal(&c, p, sizeof p); assert(sent == 1); reply(1, 0x80, 1, 64); notification(&c);
    /* Malformed signaling fragments reset assembly before the next notification. */
    ready(&c); p[0] = 0x12; p[1] = 1; p[2] = 8;
    acl(&c, 2, 5, p, 10, 12); acl(&c, 1, 0, p, 3, 0);
    assert(!sent && c.rx_reason == BLEC_RX_FRAGMENT && !c.rx_need); notification(&c);
    /* The queue survives no credits and a busy transport; feedback waits. */
    ready(&c); c.credits = 0;
    /* Two-peer contract: each LE Connection Update holds the HCI command credit
     * until its Command Status returns it. */
    for (unsigned id = 1; id <= 4; id++) {
        const uint8_t status[] = {0x0f, 4, 0, 1, 0x13, 0x20};
        request(&c, (uint8_t)id);
        assert(sent == id && last_type == 1);
        blec_rx(&c, 4, status, sizeof status, now);
    }
    assert(sent == 4 && last_type == 1); notification(&c);
    const uint8_t feedback[] = {0x90, 60, 127}; assert(blec_midi_send(&c, feedback, 3));
    unsigned held = sent;
    done(&c, 0x51, 5); blec_poll(&c, now); assert(sent == held && !c.credits);
    done(&c, 0x50, 5); busy = 1; blec_poll(&c, now); assert(sent == held && c.credits == 5);
    busy = 0;
    for (unsigned id = 1; id <= 4; id++) {
        blec_poll(&c, now); /* The last reply can share a poll with feedback. */
        if (id < 4) { assert(sent == held + id); reply(0x13, id, 0, 0); }
    }
    assert(sent == held + 5 && last[6] == 4 && last[8] == 0x52);
    assert(replies == 4 && reply_ids[0] == 1 && reply_ids[1] == 2 && reply_ids[2] == 3 && reply_ids[3] == 4);
    /* Discovery resumes after its pending signaling reply consumes the sole credit. */
    ready(&c); c.state = BLEC_DISCOVERING; c.step = 10; c.query_start = 1;
    c.deadline = now + 5000; c.credits = 0; request(&c, 9);
    assert(sent == 1 && last_type == 1 && cmd[0] == 0x13 && cmd[1] == 0x20);
    done(&c, 0x50, 1); blec_poll(&c, now);
    assert(sent == 2 && !c.att_waiting); reply(0x13, 9, 0, 0);
    done(&c, 0x50, 1); blec_poll(&c, now);
    assert(sent == 3 && c.att_waiting && last[6] == 4 && last[8] == 0x10);
    /* Duplicate in-range requests each get an update command and an acceptance. */
    ready(&c); request(&c, 7);
    { const uint8_t status[] = {0x0f, 4, 0, 1, 0x13, 0x20}; blec_rx(&c, 4, status, sizeof status, now); }
    request(&c, 7); assert(sent == 4); reply(0x13, 7, 0, 0);
    /* Queue overflow and stalled credits abort the link, with an error. */
    ready(&c); c.credits = 0;
    for (unsigned id = 1; id <= 5; id++) {
        const uint8_t status[] = {0x0f, 4, 0, 1, 0x13, 0x20};
        request(&c, (uint8_t)id);
        blec_rx(&c, 4, status, sizeof status, now);
    }
    blec_poll(&c, now); /* Disconnect waits for the returned command credit. */
    assert(c.state == BLEC_DISCONNECTING && c.errors == 1 && last_type == 1 && last[0] == 6);
    ready(&c); c.credits = 0; now = 0xfffffff0u; request(&c, 1);
    blec_poll(&c, now + 2999u); assert(blec_connected(&c));
    blec_poll(&c, now + 3000u); assert(c.state == BLEC_DISCONNECTING && c.errors == 1);
    /* Stop/start and disconnect cannot carry a reply or a fragment to the next link. */
    ready(&c); c.credits = 0; request(&c, 1); blec_stop(&c);
    blec_start(&c, now); c.state = BLEC_CONNECTED; c.step = 14; c.conn = 0x50;c.linked=1; c.credits = 5;
    unsigned before = sent; blec_poll(&c, now); assert(sent == before);
    ready(&c); c.credits = 0; request(&c, 1);
    const uint8_t down[] = {5, 4, 0, 0x50, 0, 0x13};
    blec_rx(&c, 4, down, sizeof down, now);
    c.state = BLEC_CONNECTED; c.step = 14; c.waiting = 0; c.conn = 0x51;c.linked=1;
    before = sent; blec_poll(&c, now); assert(sent == before);
    /* Pairing is refused once. The link stays up and a later control still arrives. */
    ready(&c);
    const uint8_t pair[] = {0x01, 0x03, 0x00, 0x01, 0x10, 0x00, 0x01};
    const uint8_t smp_want[] = {0x50, 0, 6, 0, 2, 0, 6, 0, 0x05, 0x05};
    acl(&c, 2, 6, pair, sizeof pair, sizeof pair);
    assert(sent == 1 && blec_connected(&c) && last_type == 2 && last_n == sizeof smp_want);
    assert(!memcmp(last, smp_want, sizeof smp_want));
    notification(&c);
    /* A peer MTU request is answered. Its response does not disconnect. */
    ready(&c);
    const uint8_t mtu_req[] = {0x02, 23, 0};
    const uint8_t mtu_rsp[] = {0x03, 64, 0};
    acl(&c, 2, 4, mtu_req, sizeof mtu_req, sizeof mtu_req);
    assert(blec_connected(&c) && sent == 1 && last_type == 2);
    assert(last[6] == 4 && last[8] == 0x03 && last[9] == 64 && last[10] == 0);
    before = sent;
    acl(&c, 2, 4, mtu_rsp, sizeof mtu_rsp, sizeof mtu_rsp);
    assert(blec_connected(&c) && sent == before);
    notification(&c);
    /* Subscribe sends one MTU request and leaves the link connected. */
    ready(&c); c.step = 13; /* S_SUBSCRIBE */
    const uint8_t wr[] = {0x13};
    acl(&c, 2, 4, wr, sizeof wr, sizeof wr);
    assert(blec_connected(&c) && c.step == 14 && sent == 1);
    assert(last_type == 2 && last[6] == 4 && last[8] == 0x02 && last[9] == 64 && last[10] == 0);
    notification(&c);
    puts("LE signaling, bounds, priority, backpressure, lifecycle: PASS");
    lifecycle();
    return 0;
}
