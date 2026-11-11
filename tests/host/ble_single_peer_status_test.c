/* SPDX-License-Identifier: GPL-3.0-only */
/* Single-SMC default: no second scanner while peer A connects; a failed A rejoins. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/ble/ble_midi_central.c"
static unsigned sends_b;
static int send_a(uint8_t t, const uint8_t *p, uint16_t n, void *ctx) { (void)t; (void)p; (void)n; (void)ctx; return 0; }
static int send_b(uint8_t t, const uint8_t *p, uint16_t n, void *ctx) { (void)t; (void)p; (void)n; (void)ctx; sends_b++; return 0; }
int main(void)
{
    const uint8_t addr[6] = {1,2,3,4,5,0xc0};
    blec_t a, b;
#if BLEC_TWO_PEER
    puts("skip: two-peer build"); return 0;
#endif
    blec_init(&a, send_a, 0, 0, addr); blec_init(&b, send_b, 0, 0, addr);
    blec_pair_bind(&a, &b);
    assert(!blec_paired(&a) && !blec_paired(&b));
    blec_start(&a, 0);
    /* A linked and discovering: the pair scheduler must not start B. */
    a.state = BLEC_DISCOVERING; a.linked = 1; a.conn = 1; a.deadline = 5000;
    for (uint32_t t = 0; t < 3000; t += 10) blec_pair_poll(&a, &b, t, 1);
    assert(b.state == BLEC_OFF && sends_b == 0);
    /* A failed attempt: it rejoins after the backoff (not stuck FAILED). */
    a.linked = 0; a.conn = 0; a.waiting = 0; a.disconnect_flags = 0; a.now_ms = 4000;
    blec_fail(&a);
    assert(a.state == BLEC_FAILED);
    blec_pair_poll(&a, &b, 4500, 1);
    assert(a.state == BLEC_FAILED);
    blec_pair_poll(&a, &b, 5001, 1);
    assert(a.state == BLEC_SCANNING);
    /* A stop request is not overridden. */
    a.state = BLEC_FAILED; a.disconnect_flags = D_STOP; a.deadline = 0;
    blec_pair_poll(&a, &b, 9000, 1);
    assert(a.state == BLEC_FAILED);
    puts("ble single-peer default / FAILED rejoin: PASS");
    return 0;
}
