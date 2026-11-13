/* SPDX-License-Identifier: GPL-3.0-only */
/* Investigation test (a74 owner test 1, PAD CLEAR, owner t83u: Loopy fully closed).
 * It runs the REAL clip code of main_fm1.c (lines 191-209, 213-564, 633-656 at 11d81a7, cut out by
 * tests/padinv_repaint_test.sh) with stubs for the hardware. After the Bluetooth clear, it drives every
 * event that can come with NO inbound clip message: link up again, notify up again (iOS CCCD
 * re-subscribe), Device Inquiry / APC40 introduction (smk_apc_host_seen), clip mode exit and entry,
 * the 16 ms cell aging, the burst close, the layout latch, the LCD map and 10 s of pad refresh.
 * Rule: no pad lights again. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define X0X_BLE 0
static volatile uint32_t fm1_ms;
static volatile uint8_t ck_start_req;
void smk_ck_midi_stop(void) {}
static void midi_out_event(uint32_t p) { (void)p; }
static uint8_t smk_pad_grid = 1;
#define SMK_PAD_OF_WHITE(w) (smk_pad_grid ? (w) : (w) < 8u ? (w) + 8u : (w) - 8u)
static uint32_t padled[16], padled_writes_lit;
static void padled_set(uint32_t pad, uint8_t r, uint8_t g, uint8_t b) { padled[pad] = (uint32_t)r << 16 | (uint32_t)g << 8 | b; }
static void padled_flush(void) { unsigned i; for (i = 0; i < 16u; i++) padled_writes_lit += padled[i] != 0u; }
/* LCD canvas stubs (smk_clip_lcd_draw) */
#define C_BLACK 0
#define C_WHITE 0xFFFF
#define C_DIM 0x4208
static void cv_begin(uint32_t w, uint32_t h, uint16_t c) { (void)w; (void)h; (void)c; }
static void cv_band(int32_t a, int32_t b) { (void)a; (void)b; }
static void cv_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c) { (void)x; (void)y; (void)w; (void)h; (void)c; }
#include "app/smk_apc40.h"
#include "../build/padinv/apc_a.inc"
#include "../build/padinv/apc_b.inc"
#include "../build/padinv/apc_c.inc"

static unsigned pads_lit(void) { unsigned i, k = 0; for (i = 0; i < 16u; i++) k += padled[i] != 0u; return k; }
static unsigned cells_lit(void) { unsigned i, k = 0; for (i = 0; i < 40u; i++) k += apc_led_velocity[i] != 0u || apc_cell_flags[i] != 0u; return k; }
static void run(uint32_t ms)    /* main loop + 2 kHz USB/TIMER5 ticks, no inbound MIDI */
{
    while (ms--) {
        fm1_ms++;
        smk_apc_usb_tick();
        if (!(fm1_ms % 16u)) { (void)smk_clip_map_refresh(); (void)smk_clip_lcd_draw(240, 240); smk_pads(); }
    }
}
static void loopy_page(void)    /* one Loopy refresh on the BLE path (ble.c -> smk_apc_usb_event) */
{
    unsigned n;
    for (n = 0; n < 16u; n++) {
        uint32_t note = (n / 4u) * 8u + n % 4u, vel = n == 5u ? 0x31u : n == 6u ? 0x60u : 0x0Bu;
        (void)smk_apc_usb_event(9u | 0x90u << 8 | note << 16 | vel << 24);
    }
    (void)smk_apc_usb_event(9u | 0x9Eu << 8 | 1u << 16 | 0x39u << 24);   /* a queued clip, blink channel 14 */
}
static int fails;
static void check(int ok, const char *what) { printf("%s %s (cells %u, pads %u)\n", ok ? "PASS" : "FAIL", what, cells_lit(), pads_lit()); fails += !ok; }

int main(void)
{
    unsigned i;
    clip_grid_mode = 1;
    smk_apc_ble_link(1, 1);
    smk_apc_host_seen();
    loopy_page();
    run(1000);
    check(pads_lit() > 0u, "0 Loopy colours on the pads");

    /* Loopy swiped away. iOS: the link ends (or CCCD 0), then iOS connects again and subscribes. */
    smk_apc_ble_link(1, 0); run(50);
    check(cells_lit() == 0u && pads_lit() == 0u, "1 notify off clears cells and pads");
    smk_apc_ble_link(0, 0); run(50);
    smk_apc_ble_link(1, 0); run(50);
    smk_apc_ble_link(1, 1); run(50);
    check(pads_lit() == 0u, "2 iOS reconnect + CCCD re-subscribe: pads stay dark");
    smk_apc_host_seen(); run(500);
    check(pads_lit() == 0u, "3 Device Inquiry / APC40 intro: pads stay dark");
    clip_grid_mode = 0; run(100); clip_grid_mode = 1; smk_apc_clip_entered(); run(500);
    check(pads_lit() == 0u, "4 clip mode exit and entry: pads stay dark");
    for (i = 0; i < 10u; i++) run(1000);
    check(pads_lit() == 0u && cells_lit() == 0u, "5 10 s of pad, LCD, aging and layout refresh: pads stay dark");

    /* Same with the link end first (no CCCD 0 before it). */
    loopy_page(); run(1000);
    smk_apc_ble_link(0, 0); run(50);
    smk_apc_ble_link(1, 0); smk_apc_ble_link(1, 1); smk_apc_host_seen(); run(5000);
    check(pads_lit() == 0u && cells_lit() == 0u, "6 link end, reconnect, re-subscribe, 5 s: pads stay dark");

    /* The only other writer: USB MIDI in (usb.c:750, a Mac on USB). It is not cleared by the BLE edges. */
    (void)smk_apc_usb_event(9u | 0x90u << 8 | 0u << 16 | 0x2Du << 24); run(100);
    printf("INFO a USB clip message after the clear lights the pads again: pads %u\n", pads_lit());

    printf("%s %d fail\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
