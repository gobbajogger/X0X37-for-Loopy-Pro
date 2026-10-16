/* SPDX-License-Identifier: GPL-3.0-only */
/* a76 (a74 owner test 1, PAD CLEAR FAIL): after Loopy goes away, no clip colour comes back
 * until Loopy is a listening host again (link up AND notify on). BEHAVIOR.md CLP-18.
 * The real BLE-MIDI host (src/ble/ble_midi_host.c), the real glue (src/ble/ble_glue.h) and the
 * real cell model (app/smk_apc40.h). service() has the order of ble.c ble_service: HCI frames,
 * then smk_apc_ble_link, then MIDI in (ble_usb_of_msg, smk_apc40_ble_colour_unheard) to the cells.
 *   cc -std=c99 -O2 -Wall -Ifirmware/src -Ifirmware/hal -Ifirmware/src/ble tests/smk_clip_ble_notify_test.c */
#include <stdio.h>
#include <string.h>
#include "x0x_ble_abi.h"
#include "ble_midi_host.c"
#include "ble_glue.h"
#include "app/smk_apc40.h"

static uint8_t vel[SMK_APC40_CLIP_COUNT], ch[SMK_APC40_CLIP_COUNT];
static uint8_t flags[SMK_APC40_CLIP_COUNT], tick[SMK_APC40_CLIP_COUNT];
static uint8_t link, now16;
static int fails;

static int fake_send(uint8_t t, const uint8_t *p, uint16_t n, void *ctx) { (void)t; (void)p; (void)n; (void)ctx; return 0; }
static void evt(bleh_t *h, const uint8_t *e, unsigned n) { bleh_rx(h, BLEH_H4_EVT, e, (uint16_t)n); }
static void cc(bleh_t *h, unsigned op)
{
    uint8_t e[9] = {0x0E, 4, 1, (uint8_t)op, (uint8_t)(op >> 8), 0, 0, 0, 0};
    if (op == 0x2002) { e[1] = 7; e[6] = 27; e[7] = 0; e[8] = 4; }
    evt(h, e, 2u + e[1]);
}
static void acl(bleh_t *h, const uint8_t *d, unsigned n)
{
    uint8_t b[64] = {0x40, 0x20, (uint8_t)(4 + n), 0, (uint8_t)n, 0, 4, 0};
    memcpy(b + 8, d, n);
    bleh_rx(h, BLEH_H4_ACL, b, (uint16_t)(8 + n));
}
static void cccd(bleh_t *h, uint8_t on) { uint8_t r[5] = {18, 7, 0, on, 0}; acl(h, r, 5); }
static void connect(bleh_t *h) { static const uint8_t c[21] = {0x3E, 19, 0x01, 0x00, 0x40, 0x00, 0x01}; evt(h, c, 21); }
static void disconnect(bleh_t *h) { static const uint8_t e[] = {0x05, 4, 0, 0x40, 0, 0x13}; evt(h, e, sizeof e); }
static void service(bleh_t *h)
{
    bleh_msg_t m;
    if (smk_apc40_link_lost(&link, (uint8_t)bleh_connected(h), (uint8_t)bleh_notifying(h)))
        smk_apc40_cells_clear(vel, ch, flags, tick);
    while (bleh_midi_next(h, &m)) {
        if (smk_apc40_ble_colour_unheard(ble_usb_of_msg(&m), (uint8_t)bleh_notifying(h)))
            continue;                       /* smk_apc_ble_event: no cell write, recorded as source 3 */
        if ((m.b[0] & 0xE0u) == 0x80u)
            smk_apc40_cell_event(vel, ch, flags, tick, m.b[1], m.b[0] & 0x0Fu,
                                 (m.b[0] & 0xF0u) == 0x80u ? 0u : m.b[2], now16);
    }
    (void)smk_apc40_cells_age(vel, ch, flags, tick, now16);   /* smk_apc_usb_tick: the 256 ms off-hold */
    now16++;
}
/* One Loopy feedback write (ATT Write Command 0x52 on the MIDI handle): note 0x20 violet, note 0x21 colour */
static void write_feedback(bleh_t *h)
{
    const uint8_t d[] = {0x80, 0x80, 0x90, 0x20, 0x45, 0x80, 0x90, 0x21, 0x50};
    uint8_t b[40] = {0x52, 6, 0};
    memcpy(b + 3, d, sizeof d);
    acl(h, b, 3 + sizeof d);
}
static unsigned lit(void)
{
    unsigned n, k = 0;
    for (n = 0; n < SMK_APC40_CLIP_COUNT; n++)
        k += vel[n] != 0u || flags[n] != 0u;
    return k;
}
static void check(int ok, const char *what)
{
    printf("%s %s (lit %u)\n", ok ? "PASS" : "FAIL", what, lit());
    fails += !ok;
}
static void up(bleh_t *h)   /* Loopy connected and listening, pads show its colours */
{
    connect(h); service(h); cccd(h, 1); service(h);
    write_feedback(h); service(h);
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
    service(&h);

    /* A. Loopy closes its input from X0X: the central writes CCCD 0, the link stays up.
     *    Loopy still sends feedback to the APC40 output (refresh, blink flash, state change). */
    up(&h);
    check(lit() == 2u, "A0 Loopy colours on the pads");
    cccd(&h, 0); service(&h);
    check(lit() == 0u, "A1 CCCD 0 clears the pads");
    write_feedback(&h); service(&h);
    check(lit() == 0u, "A2 after CCCD 0 a Loopy feedback write does not light the pads again");
    for (i = 0; i < 100u; i++) service(&h);
    check(lit() == 0u, "A3 the pads stay dark while notify stays off");

    /* B. CCCD 0 and a feedback write in the same event poll (8 frames per ble_service). */
    cccd(&h, 1); service(&h); write_feedback(&h); service(&h);
    cccd(&h, 0); write_feedback(&h); service(&h);
    check(lit() == 0u, "B  CCCD 0 and feedback in one poll: the pads stay dark");

    /* C. The link ends, iOS connects again at once (known iOS behaviour after a link loss),
     *    and a write comes before the CCCD write. */
    cccd(&h, 1); service(&h); write_feedback(&h); service(&h);
    disconnect(&h); service(&h);
    check(lit() == 0u, "C1 disconnect clears the pads");
    connect(&h); service(&h); write_feedback(&h); service(&h);
    check(lit() == 0u, "C2 reconnect without notify: a feedback write does not light the pads");

    /* D. No regression: Loopy listens again and sends feedback: the colours show. */
    cccd(&h, 1); service(&h); write_feedback(&h); service(&h);
    check(lit() == 2u, "D  notify on again with new feedback shows the colours");
    for (i = 0; i < 100u; i++) service(&h);
    check(lit() == 2u, "E  a steady link keeps the colours");

    printf("%s %d fail\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
