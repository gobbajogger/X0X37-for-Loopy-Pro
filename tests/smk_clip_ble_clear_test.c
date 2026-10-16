/* SPDX-License-Identifier: GPL-3.0-only */
/* Clip mode (a71): when Loopy goes away on Bluetooth, the clip feedback clears. The real BLE-MIDI host
 * (src/ble/ble_midi_host.c) gets HCI events and ATT writes; the cell model is app/smk_apc40.h; the glue
 * is the same as main_fm1.c smk_apc_ble_link (called by ble.c ble_service after each event poll).
 *   cc -std=c99 -O2 -Wall -Ifirmware/src -Ifirmware/src/ble tests/smk_clip_ble_clear_test.c */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ble_midi_host.c"
#include "app/smk_apc40.h"

static uint8_t vel[SMK_APC40_CLIP_COUNT], ch[SMK_APC40_CLIP_COUNT];
static uint8_t flags[SMK_APC40_CLIP_COUNT], tick[SMK_APC40_CLIP_COUNT];
static uint8_t link, now16;

static int fake_send(uint8_t t, const uint8_t *p, uint16_t n, void *ctx) { (void)t; (void)p; (void)n; (void)ctx; return 0; }
static void evt(bleh_t *h, const uint8_t *e, unsigned n) { bleh_rx(h, BLEH_H4_EVT, e, (uint16_t)n); }
static void cc(bleh_t *h, unsigned op)
{
    uint8_t e[9] = {0x0E, 4, 1, (uint8_t)op, (uint8_t)(op >> 8), 0, 0, 0, 0};
    if (op == 0x2002) { e[1] = 7; e[6] = 27; e[7] = 0; e[8] = 4; }
    evt(h, e, 2u + e[1]);
}
static void acl(bleh_t *h, const uint8_t *d, unsigned n)   /* ATT on handle 0x040 */
{
    uint8_t b[64] = {0x40, 0x20, (uint8_t)(4 + n), 0, (uint8_t)n, 0, 4, 0};
    memcpy(b + 8, d, n);
    bleh_rx(h, BLEH_H4_ACL, b, (uint16_t)(8 + n));
}
static void cccd(bleh_t *h, uint8_t on) { uint8_t r[5] = {18, 7, 0, on, 0}; acl(h, r, 5); }
static void connect(bleh_t *h)
{
    static const uint8_t cmplt[21] = {0x3E, 19, 0x01, 0x00, 0x40, 0x00, 0x01};
    evt(h, cmplt, 21);
    assert(bleh_connected(h) && !bleh_notifying(h));
}
static void disconnect(bleh_t *h)
{
    static const uint8_t e[] = {0x05, 4, 0, 0x40, 0, 0x13};   /* the central ended the link */
    evt(h, e, sizeof e);
    assert(!bleh_connected(h));
}
/* ble.c ble_service: events first, then the link check, then MIDI in to the cells */
static void service(bleh_t *h)
{
    bleh_msg_t m;
    if (smk_apc40_link_lost(&link, (uint8_t)bleh_connected(h), (uint8_t)bleh_notifying(h)))
        smk_apc40_cells_clear(vel, ch, flags, tick);
    while (bleh_midi_next(h, &m))
        if ((m.b[0] & 0xE0u) == 0x80u)
            smk_apc40_cell_event(vel, ch, flags, tick, m.b[1], m.b[0] & 0x0Fu,
                                 (m.b[0] & 0xF0u) == 0x80u ? 0u : m.b[2], now16);
}
/* Loopy feedback: note 0x20 violet 0x31 (ch 0), note 0x21 queued (blink channel 14, 0x39) */
static void feedback(bleh_t *h)
{
    const uint8_t d[] = {0x80, 0x80, 0x90, 0x20, 0x31, 0x80, 0x9E, 0x21, 0x39};
    uint8_t b[40] = {0x52, 6, 0};
    memcpy(b + 3, d, sizeof d);
    acl(h, b, 3 + sizeof d);
    service(h);
}
static unsigned lit(void)
{
    unsigned n, k = 0;
    for (n = 0; n < SMK_APC40_CLIP_COUNT; n++)
        k += vel[n] != 0u || flags[n] != 0u;
    return k;
}

int main(void)
{
    static bleh_t h;
    static const uint8_t addr[6] = {1, 2, 3, 4, 5, 6};
    static const unsigned ops[] = {0x0C03, 0x0C01, 0x2001, 0x2002, 0x2005, 0x2006, 0x2008, 0x2009};
    unsigned i;
    bleh_init(&h, fake_send, 0, "X0X-TEST", addr);
    bleh_start(&h);
    for (i = 0; i < 8u; i++)
        cc(&h, ops[i]);
    assert(h.state == BLEH_ADVERTISING);
    service(&h);

    /* 1. The link ends: the pads clear (no colour, no queued blink). */
    connect(&h); cccd(&h, 1); service(&h);
    feedback(&h);
    assert(vel[0x20] == 0x31u && vel[0x21] == 0x39u && smk_apc40_is_queued(ch[0x21]) && lit() == 2u);
    disconnect(&h); service(&h);
    assert(lit() == 0u);
    printf("PASS disconnect clears the clip pads\n");

    /* 2. A reconnect with new feedback shows the colours again. */
    connect(&h); service(&h);
    assert(lit() == 0u);
    cccd(&h, 1); service(&h);
    feedback(&h);
    assert(vel[0x20] == 0x31u && vel[0x21] == 0x39u && lit() == 2u);
    printf("PASS reconnect with new feedback shows the colours again\n");

    /* 3. The central turns notify off (CCCD 0), the link stays up: the pads clear. */
    cccd(&h, 0); service(&h);
    assert(bleh_connected(&h) && !bleh_notifying(&h) && lit() == 0u);
    cccd(&h, 1); service(&h);
    feedback(&h);
    assert(lit() == 2u);
    printf("PASS CCCD 0 clears the clip pads; notify on again with feedback shows them\n");

    /* 4. No false clear: a link that stays up keeps the feedback. */
    for (i = 0; i < 100u; i++)
        service(&h);
    assert(lit() == 2u);
    printf("PASS a steady link keeps the clip feedback\n");
    return 0;
}
