/* SPDX-License-Identifier: GPL-3.0-only */
/* Test production ingress objects. This does not instantiate two mixer records
 * or simulate a second connection. Existing extraction suites test real poll. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef SMC_INPUT_HEADER
#define SMC_INPUT_HEADER "app/smc_ble_input.h"
#endif
#include SMC_INPUT_HEADER

static smc_ble_input_t peer[2];
static const uint8_t off[3] = {0x80, 8, 0};
static const uint8_t on[3] = {0x90, 8, 127};
static const uint8_t cc[3] = {0xB0, 16, 42};
static void fill(smc_ble_input_t *p)
{
    for (unsigned i = 0; i < SMC_BLE_INPUT_CAPACITY; i++)
        assert(smc_ble_input_push(p, cc, 3, 0, 0));
}
static void unchanged(const smc_ble_input_t *p, const smc_ble_input_t *before)
{
    assert(!memcmp(p, before, sizeof *p));
}
static void one(unsigned which, uint8_t start)
{
    smc_ble_input_t *a = &peer[which], *b = &peer[which ^ 1u];
    memset(peer, 0, sizeof peer);
    a->w = a->r = b->w = b->r = start;
    smc_ble_input_link(a, 1);
    smc_ble_input_link(b, 1);
    /* Peer A floods while B queues the release of a physically held key.
     * Neither A overflow nor A reconnect may alter B's release or epoch. */
    assert(smc_ble_input_push(b, on, 3, 1, 0));
    assert(smc_ble_input_push(b, off, 3, 1, 1));
    smc_ble_input_t saved = *b;
    fill(a);
    assert(!smc_ble_input_push(a, off, 3, 0, 1));
    assert(a->release_lost == 1u);
    unchanged(b, &saved);
    /* Foreign/nonrelease overflow cannot manufacture a release loss. */
    assert(!smc_ble_input_push(a, cc, 3, 0, 0));
    assert(a->release_lost == 1u);
    smc_ble_input_link(a, 0);
    uint32_t down = a->link;
    smc_ble_input_link(a, 1);
    assert(a->link != down && (a->link & 1u));
    assert((uint8_t)(a->link >> 1) == a->w);
    assert(a->link_release_boundary == 1u);
    unchanged(b, &saved);
    smc_event_t release = b->q[(uint8_t)(start + 1u) % SMC_BLE_INPUT_CAPACITY];
    assert(release.n == (3u | SMC_EVENT_USER));
    assert(!memcmp(release.b, off, 3));

    /* Model the actual consumer's copy-before-ack rule. Refilling that exact
     * slot cannot tear the copied event. A release lost AFTER a poll snapshot
     * stays a new generation for the following poll; it is never acknowledged
     * by writing producer state. Exercise all byte cursor offsets. */
    a->r = a->w; /* discard old session at published cutoff */
    assert(smc_ble_input_push(a, on, 3, 0, 0));
    for (unsigned i = 1; i < SMC_BLE_INPUT_CAPACITY; i++)
        assert(smc_ble_input_push(a, cc, 3, 0, 0));
    uint32_t snapshot = a->release_lost;
    smc_event_t copy = a->q[a->r % SMC_BLE_INPUT_CAPACITY];
    a->r++;
    assert(smc_ble_input_push(a, cc, 3, 0, 0));
    assert(!smc_ble_input_push(a, off, 3, 0, 1));
    assert(!memcmp(copy.b, on, 3));
    assert(a->release_lost == snapshot + 1u);
    assert(a->link_release_boundary == snapshot);
    unchanged(b, &saved);
    /* A late new-session loss must remain newer than reconnect's watermark. */
    smc_ble_input_link(a, 0);
    smc_ble_input_link(a, 1);
    snapshot = a->link_release_boundary;
    assert(!smc_ble_input_push(a, off, 3, 0, 1));
    assert(a->release_lost == snapshot + 1u);
    unchanged(b, &saved);
    /* Invalid envelopes never increment loss, even if caller says release. */
    smc_ble_input_t invalid_before = *a;
    assert(!smc_ble_input_push(a, NULL, 3, 0, 1));
    assert(!smc_ble_input_push(a, off, 0, 0, 1));
    assert(!smc_ble_input_push(a, off, 7, 0, 1));
    unchanged(a, &invalid_before);
    /* Loss and link generations wrap independently of peer B. */
    a->release_lost = UINT32_MAX;
    assert(!smc_ble_input_push(a, off, 3, 0, 1));
    assert(a->release_lost == 0);
    a->link = UINT32_MAX;
    smc_ble_input_link(a, 0);
    assert(!(a->link & 1u) && !(a->link & ~0x1FFu));
    assert(a->link_release_boundary == 0);
    unchanged(b, &saved);
}
int main(void)
{
    for (unsigned who = 0; who < 2; who++)
        for (unsigned cursor = 0; cursor < 256; cursor++)
            one(who, (uint8_t)cursor);
    printf("SMC BLE ingress: 512 two-object overflow/link/refill/wrap scenarios PASS\n");
    return 0;
}
