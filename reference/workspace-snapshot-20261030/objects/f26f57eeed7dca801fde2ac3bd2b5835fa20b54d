/* SPDX-License-Identifier: GPL-3.0-only */
/* a76 (a74 owner test 1, PAD CLEAR FAIL; Loopy fully closed): after Loopy goes away on Bluetooth,
 * the pads stay dark. BEHAVIOR.md CLP-18.
 * It runs the REAL clip code of main_fm1.c and the REAL pad LED code of lcd.c (padled_set,
 * padled_flush, padled_sent), cut out by tests/smk_clip_dark_test.sh, with stubs for the hardware.
 * The pad colour stores that it checks:
 *   main_fm1.c apc_led_velocity, apc_led_channel, apc_cell_flags, apc_cell_tick (the clip cells),
 *   apc_play_feedback, apc_record_feedback, apc_burst_mask, apc_burst_count,
 *   lcd.c padled_rgb (the pad colour store), padled_buf (the LED driver data, the colours sent).
 * The pads are dark when padled_buf (decoded with padled_sent) is dark. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define X0X_BLE 0
#define SMK_BIG
static volatile uint32_t fm1_ms;
static void midi_out_event(uint32_t p) { (void)p; }
static void lcd_sync(void) {}
static void fm1_padled_send(const void *p, uint32_t n) { (void)p; (void)n; }
static void fm1_lcd_wait(void) {}
static void fm1_padled_done(void) {}
static uint8_t smk_pad_grid = 1;
#define SMK_PAD_OF_WHITE(w) (smk_pad_grid ? (w) : (w) < 8u ? (w) + 8u : (w) - 8u)
/* LCD canvas stubs (smk_clip_lcd_draw) */
#define C_BLACK 0
#define C_WHITE 0xFFFF
#define C_DIM 0x4208
static void cv_begin(uint32_t w, uint32_t h, uint16_t c) { (void)w; (void)h; (void)c; }
static void cv_band(int32_t a, int32_t b) { (void)a; (void)b; }
static void cv_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c) { (void)x; (void)y; (void)w; (void)h; (void)c; }
#include "padled.inc"
#include "app/smk_apc40.h"
#include "app/apc40_lcd565.h"     /* smk_clip_lcd_draw reads APC40_LCD565 (CLP-05) */
#include "app/apc40_lcd_idle.h"   /* and APC40_LCD_IDLE565 through smk_clip_lcd565 (CLP-05) */
#include "app/smk_clip_colour.h"
static uint32_t irq_save(void) { return 0u; }
static void irq_restore(uint32_t f) { (void)f; }
#include "apc_a.inc"
static volatile uint8_t ck_start_req;
void smk_ck_midi_stop(void) {}
static unsigned inlog_src3;         /* smk_inlog (in #if BOARD_SMK37, not cut out): count source 3 */
void smk_inlog(uint32_t src, uint32_t pkt) { (void)pkt; inlog_src3 += src == 3u; }
#include "apc_b.inc"
#include "apc_c.inc"
/* The clip keys pages (clip-keys-a65, main_fm1.c ck_page_on, ck_page_draw): the merged
 * smk_clip_lcd_draw calls them. The real ones are static and are not cut out: no page shows.
 * Without clip keys nothing calls them (the test builds with -w). */
static uint32_t ck_page_on(void) { return 0u; }
static void ck_page_draw(void) {}

static unsigned pads_lit(void)      /* the LED driver data: what the pads show */
{
    unsigned i, k = 0;
    for (i = 0; i < PADLED_N; i++)
        k += padled_sent(i) != 0u;
    return k;
}
static unsigned stores_lit(void)    /* every other pad colour store */
{
    unsigned i, k = 0;
    for (i = 0; i < SMK_APC40_CLIP_COUNT; i++)
        k += apc_led_velocity[i] != 0u || apc_led_channel[i] != 0u || apc_cell_flags[i] != 0u;
    for (i = 0; i < PADLED_N; i++)
        k += padled_rgb[i] != 0u;
    return k + (apc_play_feedback != 0u) + (apc_record_feedback != 0u) + (apc_burst_mask != 0u) +
           (apc_burst_count != 0u);
}
static unsigned cells_lit(void)     /* the clip cells only */
{
    unsigned i, k = 0;
    for (i = 0; i < SMK_APC40_CLIP_COUNT; i++)
        k += apc_led_velocity[i] != 0u || apc_led_channel[i] != 0u || apc_cell_flags[i] != 0u;
    return k;
}
static void run(uint32_t ms)        /* 1 ms ticks; the frame (smk_frame: map, LCD, pads) each 16 ms */
{
    while (ms--) {
        fm1_ms++;
        smk_apc_usb_tick();
        if (!(fm1_ms % 16u)) { (void)smk_clip_map_refresh(); (void)smk_clip_lcd_draw(240, 240); smk_pads(); }
    }
}
static uint8_t bt_n;                /* notify is on (ble.c bleh_notifying): the last link() state */
static void link(uint8_t c, uint8_t n, uint32_t ms)   /* ble_service calls smk_apc_ble_link each poll */
{
    bt_n = (uint8_t)(c && n);
    while (ms--) {
        smk_apc_ble_link(c, n);
        run(1);
    }
}
/* CLP-18: each link owns the cells that it wrote last. Loopy on Bluetooth: ble.c gives each note to
 * smk_apc_ble_event with the notify state. umsg: Loopy on USB (usb.c ep1_rx, smk_apc_usb_event). */
static void msg(uint32_t st, uint32_t note, uint32_t vel) { (void)smk_apc_ble_event((st >> 4) | st << 8 | note << 16 | vel << 24, bt_n); }
static void umsg(uint32_t st, uint32_t note, uint32_t vel) { (void)smk_apc_usb_event((st >> 4) | st << 8 | note << 16 | vel << 24); }
static void loopy_page(void)        /* one Loopy refresh on Bluetooth */
{
    unsigned n;
    for (n = 0; n < 16u; n++)
        msg(0x90u, (n / 4u) * 8u + n % 4u, n == 5u ? 0x31u : n == 6u ? 0x60u : 0x0Bu);
    msg(0x9Eu, 1u, 0x39u);          /* a queued clip, blink channel 14 */
}
static int fails;
static void check(int ok, const char *what)
{
    printf("%s %s (stores %u, pads %u)\n", ok ? "PASS" : "FAIL", what, stores_lit(), pads_lit());
    fails += !ok;
}
static void up(void) { link(1, 1, 20); loopy_page(); link(1, 1, 1000); }

int main(void)
{
    clip_grid_mode = 1;
    smk_apc_host_seen();
    up();
    check(pads_lit() > 0u, "0 Loopy colours on the pads");

    /* 1. Loopy closed: notify off, then the link ends. Only the hardware runs. 10 s. */
    link(1, 0, 50);
    check(pads_lit() == 0u && stores_lit() == 0u, "1 notify off clears every store and the pads");
    link(0, 0, 10000);
    check(pads_lit() == 0u && stores_lit() == 0u, "1 10 s after the link end: every store and the pads are dark");

    /* 2. iOS connects again and subscribes, a Device Inquiry, clip mode exit and entry. No feedback. */
    link(1, 0, 50); link(1, 1, 50); smk_apc_host_seen();
    clip_grid_mode = 0; link(1, 1, 100); clip_grid_mode = 1; smk_apc_clip_entered(); link(1, 1, 10000);
    check(pads_lit() == 0u && stores_lit() == 0u, "2 reconnect, re-subscribe, inquiry, clip mode again, 10 s: dark");

    /* 3. A late write: a clip message 100 ms after the link end (a write that was on its way, or USB). */
    up();
    link(0, 0, 100);
    loopy_page();
    link(0, 0, 10000);
    check(pads_lit() == 0u && stores_lit() == 0u, "3 a late write 100 ms after the link end: dark after 10 s");

    /* 4. The same after notify off (the link stays up). */
    link(1, 1, 20); up();
    link(1, 0, 250);
    msg(0x90u, 0u, 0x2Du);
    link(1, 0, 10000);
    check(pads_lit() == 0u && stores_lit() == 0u, "4 a late write 250 ms after notify off: dark after 10 s");

    /* 5. No regression: Loopy comes back within the 300 ms and sends its colours: they stay. */
    link(1, 1, 20); up();
    link(0, 0, 50); link(1, 0, 50); link(1, 1, 20); loopy_page();
    link(1, 1, 10000);
    check(pads_lit() > 0u, "5 Loopy back after 120 ms with new colours: the colours stay");

    /* 6. No regression: a USB clip message after the 300 ms (Loopy on USB) shows. */
    link(0, 0, 1000);
    check(pads_lit() == 0u, "6 the link end clears the pads");
    umsg(0x90u, 0u, 0x2Du);
    link(0, 0, 10000);
    check(pads_lit() > 0u, "6 a USB clip message 1 s after the link end shows");

    /* 7. Rule 8 (never drop notes): while notify is off, only a note that writes a clip cell
     *    (notes 0-39: the pad colours) does not change the cell. smk_apc_ble_event gives every
     *    message the same answer as smk_apc_usb_event (1 = APC feedback, 0 = the sound engine). */
    {
        static const uint32_t other[] = {
            9u | 0x90u << 8 | 60u << 16 | 100u << 24,   /* a note: track LED in APC terms, or a played note */
            8u | 0x80u << 8 | 60u << 16,
            9u | 0x90u << 8 | 91u << 16 | 127u << 24,   /* PLAY feedback (transport LED) */
            9u | 0x90u << 8 | 100u << 16 | 90u << 24,   /* a note above 98 */
            0xBu | 0xB0u << 8 | 16u << 16 | 5u << 24,   /* a knob ring CC */
            0xBu | 0xB0u << 8 | 7u << 16 | 99u << 24,   /* another CC */
        };
        static const uint32_t colour[] = {   /* CLP-18: only a colour stops; an off always goes to the cell */
            9u | 0x90u << 8 | 5u << 16 | 45u << 24, 9u | 0x9Eu << 8 | 1u << 16 | 0x39u << 24,
            9u | 0x90u << 8 | 39u << 16 | 1u << 24,
        };
        unsigned i, owned, ok;
        link(1, 0, 50);
        for (owned = 0; owned < 2u; owned++) {
            smk_apc_host_lost();
            clip_grid_mode = (uint8_t)owned;
            ok = 1;
            for (i = 0; i < sizeof other / sizeof other[0]; i++) {
                uint8_t r1 = smk_apc_ble_event(other[i], 0u), p1 = apc_play_feedback;
                uint8_t r2;
                apc_play_feedback = 0u;
                r2 = smk_apc_usb_event(other[i]);
                ok &= r1 == r2 && p1 == apc_play_feedback;
                apc_play_feedback = 0u;
            }
            check(ok, owned ? "7 notify off, clip mode: notes and CCs that write no pad colour act as before"
                            : "7 notify off, no clip mode: notes and CCs that write no pad colour act as before");
            inlog_src3 = 0u;
            ok = 1;
            for (i = 0; i < sizeof colour / sizeof colour[0]; i++)
                ok &= smk_apc_ble_event(colour[i], 0u) == owned;   /* 0: the note still goes to the sound engine */
            link(1, 0, 100);
            /* (this test cuts out only the clip-mode part of smk_pads: without clip mode, check the cells) */
            check(ok && cells_lit() == 0u && inlog_src3 == 3u && (!owned || (pads_lit() == 0u && stores_lit() == 0u)),
                  owned ? "7 notify off, clip mode: a pad colour note writes no cell, is recorded as source 3"
                        : "7 notify off, no clip mode: a pad colour note writes no cell and still goes to the engine");
        }
        clip_grid_mode = 1;
        link(1, 1, 50);
        (void)smk_apc_ble_event(colour[0], 1u);
        link(1, 1, 100);
        check(pads_lit() > 0u, "7 notify on: a pad colour note lights the pad as before");
    }

    /* 8. Edges (builder loop step 7). */
    clip_grid_mode = 1;
    link(1, 1, 50); up();
    link(0, 0, 100); link(1, 0, 100); link(1, 1, 90); loopy_page();   /* back at 290 ms, colours at 290 ms */
    link(1, 1, 10000);
    check(pads_lit() > 0u, "8a notify on again inside the 300 ms window, new colours: they stay 10 s");

    up();
    link(0, 0, 100);
    clip_grid_mode = 0; link(0, 0, 50);
    msg(0x90u, 0u, 0x2Du); link(0, 0, 50);                          /* a late write while clip mode is off */
    clip_grid_mode = 1; smk_apc_clip_entered(); link(0, 0, 10000);
    check(pads_lit() == 0u && stores_lit() == 0u, "8b clip mode exit and entry inside the window, a late write: dark 10 s");

    link(1, 1, 50); up();
    {
        unsigned n;
        for (n = 0; n < 8u; n++)                                    /* the first half of a page burst */
            msg(0x90u, (n / 4u) * 8u + n % 4u, 0x31u);
        link(0, 0, 2);                                              /* the link ends in the burst */
        for (n = 8; n < 16u; n++)                                   /* the rest of the burst comes late */
            msg(0x90u, (n / 4u) * 8u + n % 4u, 0x31u);
    }
    link(0, 0, 10000);
    check(pads_lit() == 0u && stores_lit() == 0u, "8c disconnect in a colour burst, the rest late: dark 10 s");

    link(1, 1, 50);
    msg(0x90u, 0u, 127u); link(1, 1, 1000);                         /* (the layout latches after 250 ms) */
    check(apc_led_velocity[0] == 127u && pads_lit() == 1u, "8d colour index 127 lights its pad");
    msg(0x90u, 0u, 0u); link(1, 1, 1000);
    check(pads_lit() == 0u && apc_led_velocity[0] == 0u && padled_rgb[0] == 0u,
          "8d colour index 0 clears its pad (the cell keeps only its 'last was off' flag)");
    msg(0x90u, 0u, 127u); link(1, 1, 100); link(0, 0, 10000);
    check(pads_lit() == 0u && stores_lit() == 0u, "8d index 127, then the link end: dark 10 s");
    link(1, 0, 50);
    (void)smk_apc_ble_event(9u | 0x90u << 8 | 0u << 16 | 127u << 24, 0u);
    (void)smk_apc_ble_event(9u | 0x90u << 8 | 39u << 16 | 127u << 24, 0u);
    link(1, 0, 100);
    check(pads_lit() == 0u && stores_lit() == 0u, "8d notify off: index 127 on notes 0 and 39 writes no cell");

    printf("%s %d fail\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
