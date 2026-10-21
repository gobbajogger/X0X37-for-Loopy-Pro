/* SPDX-License-Identifier: GPL-3.0-only */
/* Inject failures through public HCI/ACL APIs and inspect the exported evidence.
 * Reuse the connection fixture's actual wire startup/discovery, not seeded states. */
#define main connection_fixture_main
#include "ble_midi_connection_test.c"
#undef main

static unsigned kind(const blec_trace_record_t *r) { return r->context & 255u; }
static unsigned reason(const blec_trace_record_t *r) { return r->context >> 24; }
static unsigned step(const blec_trace_record_t *r) { return r->context >> 16 & 255u; }
static unsigned op(const blec_trace_record_t *r) { return r->operation & 65535u; }
static void scan(blec_t *c, uint32_t start)
{
    const uint8_t addr[6] = {1, 2, 3, 4, 5, 0xc0};
    const unsigned setup[] = {0x0c03, 0x0c01, 0x2001, 0x2002, 0x2005, 0x200b, 0x200c};
    memset(&wire, 0, sizeof wire); now = start;
    blec_init(c, send_h4, 0, 0, addr); blec_start(c, now);
    for (unsigned i = 0; i < sizeof setup / sizeof *setup; i++) cc(c, setup[i], 0);
}
static void command_recovery(void)
{
    blec_t c; name = "HCI error survives retry and successful subscription";
    scan(&c, 0); advertisement(&c); cc(&c, 0x200c, 0); cs(&c, 0x200d, 0x0c);
    CHECK(c.state == BLEC_SCANNING && c.trace.attempts == 1 && c.trace.failures == 1);
    CHECK(kind(&c.trace.first) == BLEC_TRACE_HCI_ERROR && reason(&c.trace.first) == 0x0c);
    CHECK(step(&c.trace.first) == 8 && op(&c.trace.first) == 0x200d);
    blec_trace_record_t original = c.trace.first;
    cc(&c, 0x200c, 0); advertisement(&c); cc(&c, 0x200c, 0); cs(&c, 0x200d, 0);
    connection(&c, 0, 0); discover(&c);
    CHECK(blec_connected(&c) && c.trace.attempts == 2 && c.trace.ready == 1);
    CHECK(!memcmp(&c.trace.first, &original, sizeof original));
    CHECK(!memcmp(&c.trace.latest, &original, sizeof original));
    printf("PASS %s\n", name);
}
static void controller_failure(void)
{
    blec_t c; name = "controller failure survives later empty scanning"; begin(&c, 100);
    connection(&c, 1, 0x3e);
    CHECK(kind(&c.trace.latest) == BLEC_TRACE_CONNECT_FAIL && reason(&c.trace.latest) == 0x3e);
    CHECK(op(&c.trace.latest) == 0x200d && step(&c.trace.latest) == 8);
    blec_trace_record_t original = c.trace.latest;
    cc(&c, 0x200c, 0);
    for (unsigned i = 0; i < 4; i++) timeout(&c);
    CHECK(c.state == BLEC_SCANNING && !memcmp(&c.trace.latest, &original, sizeof original));
    CHECK(c.trace.failures == 1);
    printf("PASS %s\n", name);
}
static void att_failure(void)
{
    blec_t c; name = "ATT error survives local teardown and host restart"; begin(&c, 100);
    connection(&c, 0, 0);
    const uint8_t error[] = {1, 0x10, 1, 0, 0x0a};
    acl(&c, error, sizeof error);
    CHECK(c.state == BLEC_DISCONNECTING && kind(&c.trace.latest) == BLEC_TRACE_ATT_ERROR);
    CHECK(reason(&c.trace.latest) == 0x0a && op(&c.trace.latest) == 0x10);
    CHECK(c.trace.latest.operation >> 16 == 1 && step(&c.trace.latest) == 10);
    blec_trace_record_t original = c.trace.latest;
    const uint8_t down[] = {5, 4, 0, 0x50, 0, 0x16};
    event(&c, down, sizeof down);
    CHECK(c.state == BLEC_SCANNING && c.trace.disconnects == 1 && c.trace.failures == 1);
    CHECK(!memcmp(&c.trace.latest, &original, sizeof original));
    blec_start(&c, ++now);
    CHECK(!memcmp(&c.trace.first, &original, sizeof original));
    CHECK(!memcmp(&c.trace.latest, &original, sizeof original));
    printf("PASS %s\n", name);
}
static void discovery_timeout(void)
{
    blec_t c; name = "discovery timeout identifies unanswered service request"; begin(&c, 100);
    connection(&c, 0, 0); timeout(&c);
    CHECK(c.state == BLEC_DISCONNECTING && kind(&c.trace.latest) == BLEC_TRACE_TIMEOUT);
    CHECK(reason(&c.trace.latest) == BLEC_TRACE_WAIT_ATT && op(&c.trace.latest) == 0x10);
    CHECK(step(&c.trace.latest) == 10 && (c.trace.latest.detail >> 8 & 255u) == 1);
    printf("PASS %s\n", name);

    name = "discovery timeout distinguishes an unsent characteristic request";
    begin(&c, 100); connection(&c, 0, 0);
    const uint8_t indication[] = {0x1d, 0x34, 0x12};
    for (unsigned i = 0; i < 4; i++) acl(&c, indication, sizeof indication);
    const uint8_t svc[] = {0x11, 20, 0x70, 0, 0x7f, 0,
        0x00,0xc7,0xc4,0x4e,0xe3,0x6c,0x51,0xa7,0x33,0x4b,0xe8,0xed,0x5a,0x0e,0xb8,0x03};
    acl(&c, svc, sizeof svc);
    CHECK(!c.att_waiting && !c.credits);
    timeout(&c);
    CHECK(reason(&c.trace.latest) == BLEC_TRACE_WAIT_ATT && op(&c.trace.latest) == 0x08);
    CHECK(step(&c.trace.latest) == 11 && !(c.trace.latest.detail & 0x00ffff00u));
    printf("PASS %s\n", name);
}
static void cancel_timeout(void)
{
    blec_t c; name = "Create and Cancel deadlines retained across late success"; begin(&c, 0xfffff000u);
    timeout(&c);
    CHECK(kind(&c.trace.first) == BLEC_TRACE_TIMEOUT && reason(&c.trace.first) == BLEC_TRACE_WAIT_CREATE);
    CHECK(wire.cancels == 1 && c.trace.attempts == 1);
    timeout(&c);
    CHECK(c.state == BLEC_FAILED && reason(&c.trace.latest) == BLEC_TRACE_WAIT_CANCEL);
    connection(&c, 1, 0); discover(&c);
    CHECK(blec_connected(&c) && c.trace.ready == 1 && c.trace.failures == 2);
    CHECK(reason(&c.trace.first) == BLEC_TRACE_WAIT_CREATE && reason(&c.trace.latest) == BLEC_TRACE_WAIT_CANCEL);
    printf("PASS %s\n", name);
}
static void missing_status(unsigned late_status)
{
    blec_t c; name = "missing Create status has bounded wait and owns late outcome";
    scan(&c, 0xfffff000u); advertisement(&c); cc(&c, 0x200c, 0);
    CHECK(c.waiting && wire.creates == 1);
    timeout(&c);
    CHECK(c.state == BLEC_FAILED && !c.waiting && wire.cancels == 0);
    CHECK(kind(&c.trace.first) == BLEC_TRACE_TIMEOUT && reason(&c.trace.first) == BLEC_TRACE_WAIT_HCI);
    CHECK(op(&c.trace.first) == 0x200d);
    if (late_status == 1) {
        cs(&c, 0x200d, 0x0c);
        CHECK(c.state == BLEC_SCANNING && kind(&c.trace.latest) == BLEC_TRACE_HCI_ERROR);
    } else {
        if (late_status == 2) {
            cs(&c, 0x200d, 0); blec_poll(&c, ++now);
            CHECK(c.state == BLEC_CONNECTING && wire.cancels == 1);
        }
        connection(&c, 0, 0); discover(&c);
        CHECK(blec_connected(&c) && c.trace.ready == 1);
    }
    printf("PASS %s late_status=%u\n", name, late_status);
}
static void link_loss(void)
{
    blec_t c; name = "unexpected disconnect preserves controller reason and handle"; begin(&c, 100);
    connection(&c, 0, 0); discover(&c);
    const uint8_t down[] = {5, 4, 0, 0x50, 0, 8};
    event(&c, down, sizeof down);
    CHECK(c.state == BLEC_SCANNING && c.trace.disconnects == 1);
    CHECK(kind(&c.trace.latest) == BLEC_TRACE_DISCONNECT && reason(&c.trace.latest) == 8);
    CHECK(c.trace.latest.operation >> 16 == 0x50 && step(&c.trace.latest) == 14);
    printf("PASS %s\n", name);
}
static void status_during_stop(uint32_t start, unsigned late, unsigned rejected, unsigned late_link)
{
    blec_t c; name = "late Create status retains stop deadline and command ownership";
    scan(&c, start); advertisement(&c); cc(&c, 0x200c, 0);
    CHECK(c.state == BLEC_CONNECTING && c.waiting && wire.creates == 1 && !wire.cancels);
    timeout(&c);
    CHECK(c.state == BLEC_FAILED && !wire.cancels);
    blec_stop(&c);
    CHECK(c.state == BLEC_STOPPING && !wire.cancels);
    uint32_t cutoff = c.deadline;
    if (late) { now = cutoff; blec_poll(&c, now); CHECK(c.state == BLEC_FAILED && !wire.cancels); }
    cs(&c, 0x200d, rejected ? 0x0c : 0);
    CHECK(c.deadline == cutoff && wire.creates == 1);
    if (rejected) CHECK(c.state == BLEC_OFF && !wire.cancels);
    else if (!late) {
        CHECK(c.state == BLEC_STOPPING && wire.cancels == 1);
        unsigned submitted = wire.commands;
        blec_poll(&c, now + 1);
        CHECK(wire.commands == submitted);
        cc(&c, 0x200e, 0); connection(&c, 0, 2);
        CHECK(c.state == BLEC_OFF && wire.cancels == 1);
    } else {
        CHECK(c.state == BLEC_FAILED && !wire.cancels && c.conn == BLEC_NO_CONN);
        unsigned submitted = wire.commands;
        connection(&c, 0, late_link ? 0 : 0x3e);
        CHECK(c.deadline == cutoff && wire.commands == submitted && !wire.cancels);
        if (late_link) {
            CHECK(c.state == BLEC_FAILED && c.conn == 0x50);
            const uint8_t down[] = {5, 4, 0, 0x50, 0, 0x13};
            event(&c, down, sizeof down);
            CHECK(c.state == BLEC_OFF && c.conn == BLEC_NO_CONN);
        } else CHECK(c.state == BLEC_OFF && c.conn == BLEC_NO_CONN);
    }
    printf("PASS %s wrap=%u late=%u rejected=%u late_link=%u\n", name,
           start > 0xffff0000u, late, rejected, late_link);
}
static void blocked_start(void)
{
    blec_t c; const uint8_t addr[6] = {0}; name = "mailbox refusal timeout identifies unsent Reset";
    memset(&wire, 0, sizeof wire); wire.blocked = 1; now = 0;
    blec_init(&c, send_h4, 0, 0, addr); blec_start(&c, now);
    timeout(&c);
    CHECK(c.state == BLEC_FAILED && wire.commands == 0);
    CHECK(kind(&c.trace.latest) == BLEC_TRACE_TIMEOUT && reason(&c.trace.latest) == BLEC_TRACE_WAIT_SEND);
    CHECK(op(&c.trace.latest) == 0x0c03 && c.trace.latest.detail >> 24 == 1);
    printf("PASS %s\n", name);
}
static void no_peer(void)
{
    blec_t c; name = "empty scan reports missing matching advertisement"; scan(&c, 100);
    timeout(&c);
    CHECK(c.state == BLEC_SCANNING && wire.creates == 0);
    CHECK(kind(&c.trace.latest) == BLEC_TRACE_NO_PEER && c.trace.latest.detail == 10000);
    printf("PASS %s\n", name);
}
static void peer_reply_timeout(void)
{
    blec_t c; name = "blocked ATT control reply preserves its own opcode"; begin(&c, 100);
    connection(&c, 0, 0); wire.blocked = 1;
    const uint8_t mtu[] = {0x02, 64, 0};
    acl(&c, mtu, sizeof mtu);
    now = c.att_reply_deadline; blec_poll(&c, now);
    CHECK(c.state == BLEC_DISCONNECTING && kind(&c.trace.latest) == BLEC_TRACE_TIMEOUT);
    CHECK(reason(&c.trace.latest) == BLEC_TRACE_WAIT_ATT_CONTROL && op(&c.trace.latest) == 0x03);
    CHECK(step(&c.trace.latest) == 10);
    printf("PASS %s\n", name);
}
int main(void)
{
    command_recovery(); controller_failure(); att_failure(); discovery_timeout(); cancel_timeout();
    for (unsigned i = 0; i < 3; i++) missing_status(i);
    for (unsigned wrap = 0; wrap < 2; wrap++) {
        uint32_t start = wrap ? 0xfffff000u : 100u;
        for (unsigned late = 0; late < 2; late++)
            for (unsigned rejected = 0; rejected < 2; rejected++)
                status_during_stop(start, late, rejected, 0);
        status_during_stop(start, 1, 0, 1);
    }
    link_loss(); blocked_start(); no_peer(); peer_reply_timeout();
    printf("%u failing trace scenarios\n", failures);
    return failures ? 1 : 0;
}
