/* SPDX-License-Identifier: GPL-3.0-only */
/* CLP-80 (owner 2026-10-17 02:50): "need to show waiting for loopy pro to connect instead of 8x5 grid".
 * smk_clip_wait() is 1 while no link has a Loopy host: ui.c draw_main then shows the waiting screen.
 * A link (USB or Bluetooth) has a host from the first clip or PLAY / REC feedback message on it until
 * the end of that link (smk_apc_usb_link, smk_apc_ble_link), generic mode (smk_apc_host_generic) or a
 * USB host loss (smk_apc_host_lost). A clip keys page shows also while it waits. */
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
#define C_BLACK 0
#define C_WHITE 0xFFFF
#define C_DIM 0x4208
static void cv_begin(uint32_t w, uint32_t h, uint16_t c) { (void)w; (void)h; (void)c; }
static void cv_band(int32_t a, int32_t b) { (void)a; (void)b; }
static void cv_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c) { (void)x; (void)y; (void)w; (void)h; (void)c; }
#include "padled.inc"
#include "app/smk_apc40.h"
#include "app/apc40_lcd565.h"
#include "app/apc40_lcd_idle.h"
#include "app/smk_clip_colour.h"
static uint32_t irq_save(void) { return 0u; }
static void irq_restore(uint32_t f) { (void)f; }
#include "apc_a.inc"
static volatile uint8_t ck_start_req;
void smk_ck_midi_stop(void) {}
void smk_inlog(uint32_t src, uint32_t pkt) { (void)src; (void)pkt; }
#include "apc_b.inc"
static uint32_t page;
static uint32_t ck_page_on(void) { return page; }
static void ck_page_draw(void) {}

static int fails;
static void check(int ok, const char *m)
{
    fails += !ok;
    printf("%s %s\n", ok ? "PASS" : "FAIL", m);
}
#define NOTE(n, v) (9u | 0x90u << 8 | (uint32_t)(n) << 16 | (uint32_t)(v) << 24)
#define CC(n, v) (0xBu | 0xB0u << 8 | (uint32_t)(n) << 16 | (uint32_t)(v) << 24)
static void ble_lost(void)          /* the Bluetooth link ends: the clear comes at once and 300 ms later */
{
    uint32_t t;
    for (t = 0; t < 400u; t++, fm1_ms++)
        smk_apc_ble_link(0u, 0u);
}
int main(void)
{
    uint32_t t;
    fm1_ms = 1000u;
    for (t = 0; t < 50u; t++, fm1_ms++)
        smk_apc_ble_link(1u, 1u);
    smk_apc_usb_link(1u);
    check(smk_clip_wait() == 1u, "W1 power on, links up, no message: waiting");
    smk_apc_host_seen();
    check(smk_clip_wait() == 1u, "W2 handshake only (no link given): waiting");
    (void)smk_apc_usb_event(NOTE(3, 5));
    check(smk_clip_wait() == 0u, "W3 USB clip feedback: grid");
    smk_apc_usb_link(0u);
    check(smk_clip_wait() == 1u, "W4 USB link ends: waiting");
    smk_apc_usb_link(1u);
    (void)smk_apc_usb_event(NOTE(SMK_APC40_PLAY_NOTE, 1));
    check(smk_clip_wait() == 0u, "W5 USB PLAY feedback: grid");
    smk_apc_host_lost();
    check(smk_clip_wait() == 1u, "W6 USB host lost: waiting");
    (void)smk_apc_ble_event(NOTE(9, 5), 0u);
    check(smk_clip_wait() == 1u, "W7 Bluetooth clip note with notify off: waiting");
    (void)smk_apc_ble_event(NOTE(9, 5), 1u);
    check(smk_clip_wait() == 0u, "W8 Bluetooth clip feedback: grid");
    ble_lost();
    check(smk_clip_wait() == 1u, "W9 Bluetooth link ends: waiting");
    for (t = 0; t < 50u; t++, fm1_ms++)
        smk_apc_ble_link(1u, 1u);
    (void)smk_apc_ble_event(NOTE(9, 5), 1u);
    (void)smk_apc_usb_event(NOTE(3, 5));
    ble_lost();
    check(smk_clip_wait() == 0u, "W10 USB and Bluetooth hosts, Bluetooth ends: grid (USB stays)");
    smk_apc_host_generic(0u);
    check(smk_clip_wait() == 1u, "W11 USB generic mode: waiting");
    page = 1u;
    check(smk_clip_wait() == 0u, "W12 a clip keys page shows while it waits: no waiting screen");
    page = 0u;
    clip_grid_mode = 0u;
    smk_apc_host_lost();
    smk_apc_host_seen();
    check(smk_apc_usb_event(NOTE(60, 100)) && !smk_apc_ble_event(NOTE(60, 100), 1u) &&
          smk_apc_usb_event(CC(16, 7)) && !smk_apc_ble_event(CC(16, 7), 1u),
          "W13 USB APC host cannot consume Bluetooth instrument notes and CCs");
    for (t = 0; t < 50u; t++, fm1_ms++)
        smk_apc_ble_link(1u, 1u);
    smk_apc_ble_host_seen();
    check(smk_apc_usb_event(NOTE(60, 100)) && smk_apc_ble_event(NOTE(60, 100), 1u),
          "W14 each APC host owns only its own link");
    smk_apc_host_lost();
    check(!smk_apc_usb_event(NOTE(60, 100)) && smk_apc_ble_event(NOTE(60, 100), 1u),
          "W15 USB host loss preserves Bluetooth host ownership");
    ble_lost();
    check(!smk_apc_usb_event(NOTE(60, 100)) && !smk_apc_ble_event(NOTE(60, 100), 1u),
          "W16 Bluetooth link loss releases its note ownership");
    printf("smk_clip_wait_test: %d fail(s)\n", fails);
    return fails != 0;
}
