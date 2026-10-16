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
#include "apc_a.inc"
static volatile uint8_t ck_start_req;
void smk_ck_midi_stop(void) {}
#include "apc_b.inc"
#include "apc_c.inc"

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
static void run(uint32_t ms)        /* 1 ms ticks; the frame (smk_frame: map, LCD, pads) each 16 ms */
{
    while (ms--) {
        fm1_ms++;
        smk_apc_usb_tick();
        if (!(fm1_ms % 16u)) { (void)smk_clip_map_refresh(); (void)smk_clip_lcd_draw(240, 240); smk_pads(); }
    }
}
static void link(uint8_t c, uint8_t n, uint32_t ms)   /* ble_service calls smk_apc_ble_link each poll */
{
    while (ms--) {
        smk_apc_ble_link(c, n);
        run(1);
    }
}
static void msg(uint32_t st, uint32_t note, uint32_t vel) { (void)smk_apc_usb_event((st >> 4) | st << 8 | note << 16 | vel << 24); }
static void loopy_page(void)        /* one Loopy refresh (BLE or USB: the same smk_apc_usb_event) */
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
    msg(0x90u, 0u, 0x2Du);
    link(0, 0, 10000);
    check(pads_lit() > 0u, "6 a USB clip message 1 s after the link end shows");

    printf("%s %d fail\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
