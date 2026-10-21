/* SPDX-License-Identifier: GPL-3.0-only */
/* Public-HCI startup/reconnect under temporary mailbox refusal. No physical radio. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ble_midi_central.h"

static unsigned busy, tries, accepted;
static uint8_t last[32];
static uint16_t last_n;
static uint32_t now;

static int send_h4(uint8_t type, const uint8_t *p, uint16_t n, void *ctx)
{
    (void)ctx;
    if (type == 2) return busy ? -1 : 0;
    assert(type == 1 && n <= sizeof last);
    tries++;
    if (busy) return -1;
    accepted++; last_n = n; memcpy(last, p, n);
    return 0;
}
static unsigned opcode(void) { return last[0] | (unsigned)last[1] << 8; }
static void cc(blec_t *c, unsigned op)
{
    uint8_t p[] = {0x0e, 4, 1, (uint8_t)op, (uint8_t)(op >> 8), 0, 27, 0, 5};
    if (op == 0x2002) p[1] = 7;
    blec_rx(c, 4, p, (uint16_t)(p[1] + 2u), ++now);
}
static void status(blec_t *c, unsigned op)
{
    const uint8_t p[] = {0x0f, 4, 0, 1, (uint8_t)op, (uint8_t)(op >> 8)};
    blec_rx(c, 4, p, sizeof p, ++now);
}
static void adv(blec_t *c)
{
    const uint8_t p[] = {0x3e, 17, 2, 1, 0, 1, 1, 2, 3, 4, 5, 6, 5, 4, 9, 'S', 'M', 'C', 0};
    blec_rx(c, 4, p, sizeof p, ++now);
}
static void require_submission(blec_t *c, unsigned op, unsigned refused)
{
    if (refused) {
        unsigned before = accepted;
        assert(!c->waiting);
        busy = 0; blec_poll(c, ++now);
        assert(accepted == before + 1); /* The refused command must be retried. */
    }
    assert(c->waiting && opcode() == op);
    unsigned before = tries;
    blec_poll(c, ++now);
    assert(tries == before); /* An accepted command must not be duplicated. */
}
static void scan_start(blec_t *c, unsigned fail_at, uint32_t at)
{
    const uint8_t addr[6] = {1, 2, 3, 4, 5, 0xc0};
    const unsigned ops[] = {0x0c03, 0x0c01, 0x2001, 0x2002, 0x2005, 0x200b, 0x200c};
    now = at; tries = accepted = 0; busy = fail_at == 0;
    blec_init(c, send_h4, 0, 0, addr); blec_start(c, now);
    for (unsigned i = 0; i < sizeof ops / sizeof ops[0]; i++) {
        require_submission(c, ops[i], fail_at == i);
        busy = fail_at == i + 1u && i < 6u;
        cc(c, ops[i]);
    }
    assert(c->state == BLEC_SCANNING && !c->waiting);
    unsigned before = tries; blec_poll(c, ++now); blec_poll(c, ++now);
    assert(tries == before); /* An idle scan is not a pending Scan Enable. */
}
static void connect_start(blec_t *c, unsigned fail_at, uint32_t at)
{
    scan_start(c, fail_at, at);
    busy = fail_at == 7u; adv(c);
    require_submission(c, 0x200c, fail_at == 7u);
    assert(last_n == 5 && last[3] == 0);
    busy = fail_at == 8u; cc(c, 0x200c);
    require_submission(c, 0x200d, fail_at == 8u);
    assert(c->state == BLEC_CONNECTING && last_n == 28);
    status(c, 0x200d);
    unsigned before = tries; blec_poll(c, ++now); blec_poll(c, ++now);
    assert(tries == before && c->state == BLEC_CONNECTING);
}
static void reconnect(uint32_t at)
{
    blec_t c; connect_start(&c, 99, at);
    const uint8_t up[] = {0x3e, 19, 1, 0, 0x50, 0, 0, 1, 1, 2, 3, 4, 5, 6, 24, 0, 0, 0, 0x90, 1, 0};
    const uint8_t down[] = {5, 4, 0, 0x50, 0, 8};
    blec_rx(&c, 4, up, sizeof up, ++now);
    assert(c.state == BLEC_DISCOVERING && c.att_waiting);
    busy = 1; blec_rx(&c, 4, down, sizeof down, ++now);
    assert(c.state == BLEC_SCANNING && c.conn == BLEC_NO_CONN);
    require_submission(&c, 0x200c, 1);
    assert(last_n == 5 && last[3] == 1);
    cc(&c, 0x200c);
    unsigned before = tries; blec_poll(&c, ++now);
    assert(tries == before && c.state == BLEC_SCANNING);
}
static void cutoffs(uint32_t at)
{
    blec_t c; const uint8_t addr[6] = {0};
    now = at; tries = accepted = 0; busy = 1;
    blec_init(&c, send_h4, 0, 0, addr); blec_start(&c, now);
    blec_poll(&c, now + 1u);
    unsigned before = tries; busy = 0;
    blec_poll(&c, c.deadline); blec_poll(&c, c.deadline + 1u);
    assert(c.state == BLEC_FAILED && tries == before && !accepted);

    /* If Create never reached the controller, expiry must not cancel it. */
    scan_start(&c, 99, at); adv(&c); busy = 1; cc(&c, 0x200c);
    assert(c.state == BLEC_CONNECTING && !c.waiting);
    before = accepted; busy = 0; blec_poll(&c, c.deadline);
    assert(c.state == BLEC_SCANNING && accepted == before + 1);
    assert(opcode() == 0x200c && last[3] == 1);

    /* Explicit stop can finish immediately when that Create was never submitted. */
    scan_start(&c, 99, at); adv(&c); busy = 1; cc(&c, 0x200c);
    before = tries; busy = 0; blec_stop(&c);
    assert(c.state == BLEC_OFF && tries == before);

    /* The accepted Create's automatic Cancel retries once transport recovers. */
    connect_start(&c, 99, at); busy = 1; now = c.deadline; blec_poll(&c, now);
    assert(c.state == BLEC_CONNECTING && c.step == 9 && !c.waiting);
    uint32_t original_cutoff = c.deadline;
    require_submission(&c, 0x200e, 1);
    assert(c.deadline == original_cutoff);
}
int main(int argc, char **argv)
{
    for (unsigned wrap = 0; wrap < 2; wrap++) {
        uint32_t start = wrap ? 0xfffffff0u : 100u;
        unsigned first = argc == 2 ? (unsigned)strtoul(argv[1], 0, 10) : 0;
        unsigned end = argc == 2 ? first : 8;
        for (unsigned fail_at = first; fail_at <= end; fail_at++) {
            blec_t c; connect_start(&c, fail_at, start);
        }
        if (argc == 1) { reconnect(start); cutoffs(start); }
    }
    puts("PASS: HCI submission retry, no duplicate commands, and original cutoffs");
    return 0;
}
