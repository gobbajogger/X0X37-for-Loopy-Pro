/* SPDX-License-Identifier: GPL-3.0-only */
/* Owner report 2026-10-17 01:28 on f9bcc46, item 1: Loopy on the Mac over USB. When the owner
 * pulls the USB cable, the clip pads keep Loopy's last colours. After a link loss the pads must go
 * dark, as on Bluetooth (BEHAVIOR.md CLP-18).
 * It runs the REAL usb.c (usb_poll at 2 kHz, ep1_rx, midi_link_check) on a model of the SIE
 * registers (smk_clip_usb_clear_fake_usb.h), and the REAL clip and pad LED code
 * (tests/host/smk_clip_usb_clear_test.sh cuts it out of main_fm1.c and lcd.c).
 * A host is there: the SOF frame number moves each 1 ms. The cable is out: the frame number
 * stops, no SOF comes, and the SIE gives the suspend interrupt (bus idle > 3 ms).
 * U1 fails on f9bcc46: usb.c sets usb.suspended (unplug, host asleep) but calls no clip clear;
 * only SET_CONFIGURATION 0 and a bus reset call smk_apc_host_lost. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define BOARD_SMK37 1
#define X0X_BLE 0
#define SMK_BIG
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
#include "src/usb.c"
#undef BOARD_SMK37                 /* main_fm1.c: leave out the INLOG reply (it needs editor.c), as in smk_clip_dark_test */
static volatile uint32_t fm1_ms;
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
#include "apc_a.inc"
static volatile uint8_t ck_start_req;
void smk_ck_midi_stop(void) {}
void smk_inlog(uint32_t src, uint32_t pkt) { (void)src; (void)pkt; }
#include "apc_b.inc"
#include "apc_c.inc"
static uint32_t ck_page_on(void) { return 0u; }
static void ck_page_draw(void) {}

static unsigned pads_lit(void)      /* the LED driver data: what the pads show */
{
    unsigned i, k = 0;
    for (i = 0; i < PADLED_N; i++)
        k += padled_sent(i) != 0u;
    return k;
}
static unsigned cells_lit(void)
{
    unsigned i, k = 0;
    for (i = 0; i < SMK_APC40_CLIP_COUNT; i++)
        k += apc_led_velocity[i] != 0u;
    return k;
}
static uint8_t host;                /* 1: a host is on the cable and awake */
static void run(uint32_t ms)        /* usb_poll at 2 kHz; the frame (map, LCD, pads) each 16 ms */
{
    while (ms--) {
        fm1_ms++;
        if (host) {
            fake_frame = (uint16_t)((fake_frame + 1u) & 0x7FFu);
            fake_sof = 1u;
        }
        usb_poll();
        usb_poll();
        if (!(fm1_ms % 16u)) { (void)smk_clip_map_refresh(); (void)smk_clip_lcd_draw(240, 240); smk_pads(); }
    }
}
static void usb_packet(const uint32_t *pkt, unsigned n)   /* one EP1 OUT packet from the host */
{
    unsigned i;
    for (i = 0; i < n; i++)
        memcpy(ep1rx + 4u * i, &pkt[i], 4u);
    fake_reg[1][22] = (uint8_t)(4u * n);
    fake_reg[1][23] = 0u;
    fake_rx = 1u;
    run(1);
}
static void loopy_page(void)        /* Loopy's clip colours: 16 cells of a 4x4 project */
{
    uint32_t p[16];
    unsigned n;
    for (n = 0; n < 16u; n++)
        p[n] = 9u | 0x90u << 8 | (uint32_t)((n / 4u) * 8u + n % 4u) << 16 |
               (uint32_t)(n == 5u ? 0x31u : n == 6u ? 0x60u : 0x0Bu) << 24;
    usb_packet(p, 16u);
}
static void plug(void)              /* enumerated: usb.c SET_CONFIGURATION 1 */
{
    host = 1u;
    usb.up = 1u;
    usb.config = 1u;
    ep1_config();
    run(200);
    smk_apc_host_seen();            /* Loopy: Device Inquiry and the APC40 introduction */
}
static int fails;
static void check(int ok, const char *what)
{
    printf("%s %s (cells %u, pads %u)\n", ok ? "PASS" : "FAIL", what, cells_lit(), pads_lit());
    fails += !ok;
}

int main(void)
{
    clip_grid_mode = 1;
    plug();
    loopy_page();
    run(1000);
    check(pads_lit() > 0u && !usb.suspended, "U0 Loopy over USB: its colours are on the pads");

    run(10000);
    check(pads_lit() > 0u, "U0 a steady host keeps the colours for 10 s");

    /* U1. The cable is out: no SOF, the frame number stops, the suspend interrupt comes. */
    host = 0u;
    fake_intrusb = 0x01u;
    run(1000);
    check(usb.suspended, "U1 usb.c sees the link loss (usb.suspended)");
    check(pads_lit() == 0u && cells_lit() == 0u, "U1 1 s after the USB cable is out: the pads are dark");
    run(9000);
    check(pads_lit() == 0u && cells_lit() == 0u, "U1 10 s after the USB cable is out: the pads are dark");

    /* U2 (works on f9bcc46): the cable goes in again. A bus reset clears the cells. */
    host = 1u;
    fake_intrusb = 0x04u;
    run(100);
    plug();
    check(pads_lit() == 0u && cells_lit() == 0u, "U2 cable in again: bus reset and enumeration, no feedback: dark");

    /* U3 (works on f9bcc46): Loopy back over USB: its new colours show. */
    loopy_page();
    run(1000);
    check(pads_lit() > 0u, "U3 Loopy feedback after the new enumeration shows");

    printf("%s %d fail\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
