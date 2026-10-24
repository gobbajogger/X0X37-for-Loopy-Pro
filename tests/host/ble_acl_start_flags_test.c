/* SPDX-License-Identifier: GPL-3.0-only
 * BLE-HCI ACL start PB flags: preserve received MIDI Note Off and bounds.
 * Compiles the actual ble_midi_host.c, not extracted functions.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_host.h"

static unsigned checks, failures, frames;
#define CHECK(x) do { checks++; if (!(x)) { failures++; fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); } } while (0)

static int sink(uint8_t type, const uint8_t *payload, uint16_t n, void *ctx)
{
    (void)payload; (void)ctx;
    CHECK(n < 128u);
    CHECK(type == BLEH_H4_ACL || type == BLEH_H4_CMD);
    frames++;
    return 0;
}
static void init(bleh_t *h)
{
    bleh_init(h, sink, 0, "X0X37", 0);
    h->state = BLEH_CONNECTED;
    h->conn = 0x0041u;
    h->credits = 4u;
}
static void acl(bleh_t *h, unsigned pb, unsigned connection, const uint8_t *p, unsigned n)
{
    uint8_t b[64];
    CHECK(n <= sizeof b - 4u);
    if (n > sizeof b - 4u) return;
    b[0] = (uint8_t)connection;
    b[1] = (uint8_t)((connection >> 8u) | (pb << 4u));
    b[2] = (uint8_t)n;
    b[3] = (uint8_t)(n >> 8u);
    memcpy(b + 4, p, n);
    bleh_rx(h, BLEH_H4_ACL, b, (uint16_t)(n + 4u));
}
static void no_msg(bleh_t *h)
{
    bleh_msg_t msg;
    CHECK(!bleh_midi_next(h, &msg));
}
static void note_off(bleh_t *h, unsigned note)
{
    bleh_msg_t msg = {0};
    int found = bleh_midi_next(h, &msg);
    CHECK(found == 1);
    if (found != 1) return;
    CHECK(msg.len == 3u);
    CHECK(msg.b[0] == 0x80u);
    CHECK(msg.b[1] == note);
    CHECK(msg.b[2] == 0u);
    no_msg(h);
}
static void test_complete_first(unsigned pb)
{
    bleh_t h;
    /* L2CAP length=8, CID_ATT=4, ATT Write Command handle 6,
     * BLE-MIDI header 0x80, low timestamp 0x80, Note Off 0x80 0x3c 0x00. */
    static const uint8_t p[] = {8,0,4,0,0x52,6,0,0x80,0x80,0x80,60,0};
    init(&h);
    acl(&h, pb, 0x41, p, sizeof p);
    note_off(&h, 60u);
    CHECK(h.rx_on == 0u);
}
static void test_fragmented_first(unsigned pb)
{
    bleh_t h;
    static const uint8_t p[] = {8,0,4,0,0x52,6,0,0x80,0x80,0x80,61,0};
    init(&h);
    acl(&h, pb, 0x41, p, 6u);
    CHECK(h.rx_on == 1u);
    CHECK(h.rx_have == 6u && h.rx_need == 12u);
    no_msg(&h);
    acl(&h, 1u, 0x41u, p + 6u, sizeof p - 6u);
    note_off(&h, 61u);
    CHECK(h.rx_on == 0u);
}
static void test_bad_and_boundary(void)
{
    bleh_t h;
    static const uint8_t p[] = {8,0,4,0,0x52,6,0,0x80,0x80,0x80,62,0};
    static const uint8_t too_large[] = {24,0,4,0};
    static const uint8_t wrong_cid[] = {8,0,9,0};
    static const uint8_t extraneous[] = {1,2,3,4,5,6,7};
    init(&h);
    acl(&h, 1u, 0x41u, p + 6, sizeof p - 6u); /* orphan continuation */
    no_msg(&h);
    acl(&h, 0u, 0x42u, p, sizeof p); /* wrong connection handle */
    no_msg(&h);
    acl(&h, 0u, 0x41u, too_large, sizeof too_large); /* L2CAP > ATT MTU */
    CHECK(h.rx_on == 0u);
    acl(&h, 0u, 0x41u, wrong_cid, sizeof wrong_cid);
    CHECK(h.rx_on == 0u);
    acl(&h, 0u, 0x41u, p, 6u);
    CHECK(h.rx_on == 1u);
    acl(&h, 1u, 0x41u, extraneous, sizeof extraneous); /* overshoots assembled length */
    CHECK(h.rx_on == 0u);
    no_msg(&h);
    acl(&h, 3u, 0x41u, p, 6u); /* complete flag must not leave partial PDU armed */
    CHECK(h.rx_on == 0u);
    acl(&h, 2u, 0x41u, p, sizeof p); /* recovery via flushable first */
    note_off(&h, 62u);
    acl(&h, 0u, 0x41u, p, 6u);
    CHECK(h.rx_on == 1u);
    acl(&h, 0u, 0x41u, p, sizeof p); /* new first resets incomplete old PDU */
    note_off(&h, 62u);
    CHECK(h.rx_on == 0u);
}
int main(void)
{
    test_complete_first(0u);  /* first non-flushable */
    test_complete_first(2u);  /* first flushable */
    test_complete_first(3u);  /* complete PDU: no continuation */
    test_fragmented_first(0u);
    test_fragmented_first(2u);
    test_bad_and_boundary();
    printf("BLE ACL start PB flags: %u checks, %u failures, controller frames=%u\n",checks,failures,frames);
    return failures ? 1 : 0;
}
