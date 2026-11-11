/* SPDX-License-Identifier: GPL-3.0-only
 * BLE L2CAP: zero controller ACL credits must not result in a transmitted
 * signalling request (or committed pending connection-parameter state).
 *
 * Native full-source example:
 *   cc -std=c99 -O1 -Wall -Wextra -Werror -Ifirmware/src/ble \
 *     tests/host/ble_acl_zero_credit_test.c firmware/src/ble/ble_midi_host.c -o /tmp/ble_acl_zero_credit_test
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_host.h"

static unsigned checks, failures;
#define CHECK(x) do { checks++; if (!(x)) { failures++; fprintf(stderr, "FAIL %u: %s\n", (unsigned)__LINE__, #x); } } while (0)

typedef struct {
    unsigned attempts, accepted;
    unsigned len;
    uint8_t type, frame[40];
    int reject;
} bus_t;

static int send_frame(uint8_t type, const uint8_t *p, uint16_t n, void *ctx)
{
    bus_t *b = ctx;
    b->attempts++;
    if (b->reject) return -1;
    if (n > sizeof b->frame) return -1;
    memcpy(b->frame, p, n);
    b->type = type; b->len = n; b->accepted++;
    return 0;
}

static void setup(bleh_t *h, bus_t *b, unsigned credits)
{
    memset(h, 0, sizeof *h);
    memset(b, 0, sizeof *b);
    h->send = send_frame;
    h->ctx = b;
    h->state = BLEH_CONNECTED;
    h->conn = 0x0040;
    h->credits = (uint8_t)credits;
    h->acl_total = 4;
}

static void zero_credits_refuses_to_send(void)
{
    bleh_t h; bus_t b;
    setup(&h, &b, 0);
    CHECK(bleh_conn_update(&h, 32, 64, 0, 200) == 0);
    CHECK(b.attempts == 0 && b.accepted == 0);
    CHECK(h.credits == 0 && h.cpu_state == 0);
    CHECK(bleh_conn_update(&h, 32, 64, 0, 200) == 0);
    CHECK(b.attempts == 0);
}

static void available_credit_and_no_duplicate(void)
{
    bleh_t h; bus_t b;
    setup(&h, &b, 1);
    CHECK(bleh_conn_update(&h, 32, 64, 0, 200) == 1);
    CHECK(b.attempts == 1 && b.accepted == 1 && h.credits == 0);
    CHECK(h.cpu_state == 1 && h.cpu_result == 0);
    CHECK(b.type == BLEH_H4_ACL && b.len == 20);
    if (b.len == 20) CHECK(b.frame[8] == 0x12 && b.frame[9] == 0x31);
    CHECK(bleh_conn_update(&h, 32, 64, 0, 200) == 0 && b.attempts == 1);
}

static void mailbox_rejection_does_not_consume_credit(void)
{
    bleh_t h; bus_t b;
    setup(&h, &b, 1); b.reject = 1;
    CHECK(bleh_conn_update(&h, 32, 64, 0, 200) == 0);
    CHECK(b.attempts == 1 && b.accepted == 0);
    CHECK(h.credits == 1 && h.cpu_state == 0);
    b.reject = 0;
    CHECK(bleh_conn_update(&h, 32, 64, 0, 200) == 1);
    CHECK(b.attempts == 2 && b.accepted == 1 && h.credits == 0);
    CHECK(h.cpu_state == 1);
}

static void invalid_state_does_not_send(void)
{
    bleh_t h; bus_t b;
    setup(&h, &b, 2);
    h.state = BLEH_ADVERTISING;
    CHECK(bleh_conn_update(&h, 32, 64, 0, 200) == 0 && b.attempts == 0);
}

int main(void)
{
    zero_credits_refuses_to_send();
    available_credit_and_no_duplicate();
    mailbox_rejection_does_not_consume_credit();
    invalid_state_does_not_send();
    printf("ble ACL zero credit: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
