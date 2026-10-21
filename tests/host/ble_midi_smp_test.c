/* SPDX-License-Identifier: GPL-3.0-only */
/* Hardware-log-derived Security Manager frame, simulated HCI transport only. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_central.h"

static uint8_t wire[64];
static unsigned sends, last_n, midi_count;
static uint8_t last_type;
static int transport_busy;
static int send_h4(uint8_t type, const uint8_t *p, uint16_t n, void *ctx)
{
    (void)ctx;
    if (transport_busy) return -1;
    assert(n <= sizeof wire);
    memcpy(wire, p, n); last_n = n; last_type = type; sends++;
    return 0;
}
static void midi(const uint8_t *p, uint8_t n, void *ctx)
{
    (void)ctx;
    const uint8_t expected[] = {0xb0, 16, 1};
    assert(n == sizeof expected && !memcmp(p, expected, n));
    midi_count++;
}
static void ready(blec_t *c)
{
    static const uint8_t addr[6] = {0};
    blec_init(c, send_h4, midi, 0, addr);
    c->state = BLEC_CONNECTED; c->step = 14; c->conn = 0x50;
    c->midi_handle = 114; c->cccd_handle = 115;
    c->credits = c->acl_total = 5;
    sends = last_n = midi_count = 0; transport_busy = 0;
}
static void acl(blec_t *c, uint8_t cid, const uint8_t *payload, unsigned n, uint32_t at)
{
    uint8_t p[64] = {0};
    p[0] = (uint8_t)c->conn; p[1] = (uint8_t)(0x20u | c->conn >> 8);
    assert(n <= 56);
    p[2] = (uint8_t)(n + 4);
    p[4] = (uint8_t)n;
    p[6] = cid;
    memcpy(p + 8, payload, n);
    blec_rx(c, 2, p, (uint16_t)(n + 8), at);
}
static void assert_smp_trace(const blec_t *c, unsigned opcode, unsigned length,
                             unsigned safe_status, uint32_t at)
{
    unsigned i = (c->trace.next + BLEC_TRACE_RECENT - 1u) % BLEC_TRACE_RECENT;
    const blec_trace_record_t *r = &c->trace.recent[i];
    assert((r->context & 255u) == BLEC_TRACE_SMP);
    assert((r->context >> 24) == safe_status);
    assert(r->operation == (opcode | 0x50u << 16));
    assert(r->detail == length && r->ms == at);
    assert(!memcmp(&c->trace.last_smp, r, sizeof *r));
    assert(c->trace.smp_seen & (1u << (opcode < 31u ? opcode : 31u)));
    if (opcode == 11u && length == 2u)
        assert(c->trace.smp_security == (0x50u << 16 | 0x100u | safe_status));
    else assert(!c->trace.smp_security);
    if (opcode == 5u && length == 2u)
        assert(c->trace.smp_failure == (0x50u << 16 | 0x100u | safe_status));
    else assert(!c->trace.smp_failure);
    assert(!c->trace.failures); /* Receiving SMP is not itself a failure. */
}

static void assert_pairing_not_supported(void)
{
    static const uint8_t expected[] = {0x50,0, 6,0, 2,0, 6,0, 5,5};
    assert(last_type == 2 && last_n == sizeof expected);
    assert(memcmp(wire, expected, sizeof expected) == 0);
}
static void notification(blec_t *c)
{
    const uint8_t msg[] = {0x1b,114,0,0x80,0x81,0xb0,16,1};
    acl(c, 4, msg, sizeof msg, 120);
    assert(midi_count == 1 && c->rx_notify == 1 && c->rx_midi == 1);
}
int main(void)
{
    blec_t c;
    /* The saved device header is 50 20 06 00 02 00 06 00. Its payload
     * was not captured; inject a valid two-byte SMP Security Request. */
    const uint8_t security_request[] = {0x0b, 0x00};
    const uint8_t failed[] = {0x05, 0x05};
    ready(&c);
    acl(&c, 6, security_request, sizeof security_request, 100);
    assert(sends == 1 && !c.rx_reject && !c.sig_count);
    assert_pairing_not_supported();
    assert_smp_trace(&c, 0x0b, 2, 0, 100);
    notification(&c);

    /* Peer Pairing Failed is terminal; do not ping-pong responses. */
    ready(&c);
    acl(&c, 6, failed, sizeof failed, 100);
    assert(sends == 0 && c.sig_count == 0);
    assert_smp_trace(&c, 5, 2, 5, 100);
    /* Reserved codes must be ignored rather than answered. */
    const uint8_t reserved[] = {0x0a,0};
    acl(&c, 6, reserved, sizeof reserved, 101);
    assert(sends == 0 && c.sig_count == 0);

    /* Reassemble SMP across HCI ACL fragments before deciding on a reply. */
    ready(&c);
    const uint8_t smp_first[] = {0x50,0x20,5,0,2,0,6,0,0x0b};
    const uint8_t smp_last[] = {0x50,0x10,1,0,0};
    blec_rx(&c, 2, smp_first, sizeof smp_first, 100);
    assert(sends == 0 && c.rx_need == 2 && c.rx_have == 1);
    assert(c.trace.events == 0); /* No partial-PDU trace or payload leak. */
    blec_rx(&c, 2, smp_last, sizeof smp_last, 101);
    assert(sends == 1 && !c.rx_need && !c.rx_have);
    assert_smp_trace(&c, 0x0b, 2, 0, 101);
    assert_pairing_not_supported();
    notification(&c);

    /* Preserve L2CAP signaling order when its answer precedes an SMP refusal.
     * Both share the established four-entry, credit-limited response queue. */
    ready(&c); c.credits = 0;
    const uint8_t le_update[] = {0x12,0x42,8,0,6,0,12,0,0,0,0x90,1};
    acl(&c, 5, le_update, sizeof le_update, 100);
    acl(&c, 6, security_request, sizeof security_request, 101);
    assert(sends == 0 && c.sig_count == 2);
    const uint8_t completed[] = {0x13,5,1,0x50,0,1,0};
    blec_rx(&c, 4, completed, sizeof completed, 110);
    blec_poll(&c, 111);
    assert(sends == 1 && c.sig_count == 1 && last_type == 2);
    assert(last_n == 14 && wire[6] == 5 && wire[8] == 0x13 && wire[9] == 0x42);
    blec_rx(&c, 4, completed, sizeof completed, 112);
    blec_poll(&c, 113);
    assert(sends == 2 && !c.sig_count);
    assert_pairing_not_supported();
    notification(&c);

    /* No ACL credits: bounded pending response survives and retries on poll. */
    ready(&c); c.credits = 0;
    acl(&c, 6, security_request, sizeof security_request, 100);
    assert(sends == 0 && c.sig_count == 1);
    blec_poll(&c, 101);
    assert(sends == 0 && c.sig_count == 1);
    blec_rx(&c, 4, completed, sizeof completed, 110);
    blec_poll(&c, 111);
    assert(sends == 1 && !c.sig_count);
    assert_pairing_not_supported();
    notification(&c);

    /* A blocked controller send must not discard the queued response. */
    ready(&c); transport_busy = 1;
    acl(&c, 6, security_request, sizeof security_request, 100);
    assert(sends == 0 && c.sig_count == 1);
    transport_busy = 0; blec_poll(&c, 110);
    assert(sends == 1 && c.sig_count == 0);
    assert_pairing_not_supported();

    /* Other (nonsensical) two-byte SMP attempts also get explicit refusal.
     * Do not add pairing, key exchange, bonds or flash writes. */
    ready(&c);
    const uint8_t unexpected[] = {0x01,0};
    acl(&c, 6, unexpected, sizeof unexpected, 100);
    assert(sends == 1 && !c.rx_reject);
    assert_pairing_not_supported();

    /* Unsupported L2CAP channels retain the existing bounded rejection. */
    ready(&c);
    acl(&c, 7, security_request, sizeof security_request, 100);
    assert(sends == 0 && c.rx_reject == 1 && c.rx_reason == BLEC_RX_CID);
    /* Exhaust every opcode with truncated, exact-two-byte and longer payloads.
     * All payload bytes use a marker. Only the two explicitly allowed fields
     * may appear in trace status; confirms/randoms/keys and malformed payloads
     * retain opcode/length only. Protocol decisions above remain unchanged. */
    for (unsigned opcode = 0; opcode < 256u; opcode++) {
        const unsigned lengths[] = {1, 2, 17, 56};
        for (unsigned j = 0; j < sizeof lengths / sizeof lengths[0]; j++) {
            uint8_t secret[56];
            memset(secret, 0xa5, sizeof secret); secret[0] = (uint8_t)opcode;
            ready(&c);
            acl(&c, 6, secret, lengths[j], 123);
            assert_smp_trace(&c, opcode, lengths[j],
                lengths[j] == 2 && (opcode == 0x0b || opcode == 5) ? 0xa5 : 0, 123);
        }
    }
    /* Keep the decisive metadata after ring replacement, discovery/start events,
     * disconnect and explicit retry. Only host initialization clears the latch. */
    ready(&c);
    acl(&c, 6, security_request, sizeof security_request, 0);
    blec_trace_record_t saved = c.trace.last_smp; /* timestamp zero is valid */
    for (unsigned i = 0; i < 8; i++)
        blec_trace_note(&c.trace, i + 1, BLEC_CONNECTED, 14,
                        BLEC_TRACE_READY, 0, 0x13, c.conn, 0, 0);
    for (unsigned i = 0; i < BLEC_TRACE_RECENT; i++)
        assert((c.trace.recent[i].context & 255u) != BLEC_TRACE_SMP);
    assert(!memcmp(&saved, &c.trace.last_smp, sizeof saved));
    blec_stop(&c);
    const uint8_t disconnected[] = {5, 4, 0, 0x50, 0, 0x13};
    blec_rx(&c, 4, disconnected, sizeof disconnected, 100);
    blec_start(&c, 101);
    assert(!memcmp(&saved, &c.trace.last_smp, sizeof saved));
    uint32_t exported[BLEC_SMP_TRACE_WORDS + 2u] = {0xdeadbeef};
    exported[BLEC_SMP_TRACE_WORDS + 1u] = 0xc0ffee;
    blec_smp_trace_words(&c.trace, exported + 1);
    assert(exported[0] == 0xdeadbeef && exported[BLEC_SMP_TRACE_WORDS + 1u] == 0xc0ffee);
    assert(exported[1] == BLEC_SMP_TRACE_MAGIC && exported[BLEC_SMP_TRACE_WORDS] == BLEC_SMP_TRACE_WORDS);
    assert(!memcmp(exported + 2, &saved, sizeof saved));
    ready(&c);
    acl(&c, 6, security_request, 2, 0);
    acl(&c, 6, failed, 2, 5);
    const uint8_t secret_random[] = {4, 0xa5, 0xa5, 0xa5};
    acl(&c, 6, secret_random, sizeof secret_random, 9);
    assert(c.trace.last_smp.operation == (4u | 0x50u << 16));
    assert(c.trace.last_smp.context >> 24 == 0);
    assert(c.trace.smp_security_ms == 0 && c.trace.smp_security == (0x50u << 16 | 0x100u));
    assert(c.trace.smp_failure_ms == 5 && c.trace.smp_failure == (0x50u << 16 | 0x105u));
    assert(c.trace.smp_seen == ((1u << 11) | (1u << 5) | (1u << 4)));
    blec_start(&c, 50);
    assert(c.trace.smp_security && c.trace.smp_failure);
    ready(&c);
    const blec_trace_record_t empty = {0};
    assert(!memcmp(&c.trace.last_smp, &empty, sizeof empty));
    assert(!c.trace.smp_seen && !c.trace.smp_security && !c.trace.smp_failure);
    /* Handle zero, timestamp zero and zero-valued fields are all valid data. */
    c.conn = 0;
    const uint8_t zero_failed[] = {5, 0};
    acl(&c, 6, security_request, 2, 0);
    acl(&c, 6, zero_failed, 2, 0);
    assert(c.trace.smp_security == 0x100 && c.trace.smp_failure == 0x100);
    assert(!c.trace.smp_security_ms && !c.trace.smp_failure_ms);
    /* Malformed later packets do not erase previously valid field latches. */
    acl(&c, 6, security_request, 1, 10);
    acl(&c, 6, zero_failed, 1, 11);
    assert(c.trace.smp_security == 0x100 && c.trace.smp_failure == 0x100);
    puts("PASS: SMP refusal behavior, privacy, independent security/failure latches, ring/retry/init and zero-value validity");
    return 0;
}
