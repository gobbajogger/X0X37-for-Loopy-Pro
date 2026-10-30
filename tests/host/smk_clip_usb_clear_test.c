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
 * only SET_CONFIGURATION 0 and a bus reset call smk_apc_host_lost.
 * Cases X and L (owner 2026-10-17 01:46): the owner uses Loopy on the Mac (USB) and Loopy on the iPhone
 * (Bluetooth) at the same time. The link that wrote a cell last owns it. The end of a link clears only the
 * cells that it owns; the cells of the other link stay on the pads (CLP-18).
 * Case P (owner 02:00): the PLAY and REC LED feedback follow the same rule. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define FM1_IRQ_TIMER5 63u
static uint32_t fm1_irq_mask_save(unsigned n) { (void)n; return 0; }
static void fm1_irq_mask_restore(unsigned n, uint32_t s) { (void)n; (void)s; }
#define BOARD_SMK37 1
#define X0X_BLE 0
#define SMK_BIG
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
void smc_mmc_host(uint8_t cmd, uint8_t src) { (void)cmd; (void)src; }
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
#include "app/apc40_lcd_idle.h"
#include "app/smk_clip_colour.h"
static uint32_t irq_save(void) { return 0u; }
static void irq_restore(uint32_t f) { (void)f; }
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
static uint8_t bt_up, bt_notify;   /* the Bluetooth link: ble.c ble_service gives it each poll */
static void run_bt(uint32_t ms)     /* run, and smk_apc_ble_link each 1 ms as ble_service does */
{
    while (ms--) {
        smk_apc_ble_link(bt_up, bt_notify);
        run(1);
    }
}
static void bt_note(uint32_t note, uint32_t vel)   /* a Bluetooth note from the iPhone (ble.c ble_service) */
{
    (void)smk_apc_ble_event(9u | 0x90u << 8 | note << 16 | vel << 24, bt_notify);
}
static void usb_note(uint32_t note, uint32_t vel)   /* a USB note from the Mac (usb.c ep1_rx) */
{
    uint32_t p = 9u | 0x90u << 8 | note << 16 | vel << 24;
    usb_packet(&p, 1u);
}
static unsigned lit_in(uint32_t first, uint32_t n)   /* lit cells first .. first + n - 1 */
{
    unsigned k = 0;
    for (; n; n--, first++)
        k += apc_led_velocity[first] != 0u;
    return k;
}
static void both_pages(void)        /* the Mac (USB) lights cells 0-3, the iPhone (Bluetooth) cells 8-11 */
{
    uint32_t n;
    for (n = 0; n < 4u; n++) {
        usb_note(n, 0x0Bu);
        bt_note(8u + n, 0x31u);
    }
    run_bt(1000);
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

    /* X. Both links at the same time (the owner setup): Loopy on the Mac over USB and Loopy on the iPhone
     *    over Bluetooth. The link that wrote a cell last owns it. A link end clears only its own cells. */
    fake_intrusb = 0x04u;           /* start with no cells: a bus reset clears the U3 colours (USB) */
    run(100);
    plug();
    bt_up = bt_notify = 1u;
    run_bt(50);
    both_pages();
    check(lit_in(0u, 4u) == 4u && lit_in(8u, 4u) == 4u && pads_lit() == 8u, "X0 USB cells 0-3 and Bluetooth cells 8-11 on the pads");
    host = 0u;
    fake_intrusb = 0x01u;
    run_bt(1000);
    check(lit_in(0u, 4u) == 0u && lit_in(8u, 4u) == 4u && pads_lit() == 4u,
          "X1 USB cable out: the USB cells go dark, the Bluetooth cells stay on the pads");
    run_bt(9000);
    check(lit_in(8u, 4u) == 4u && pads_lit() == 4u, "X1 10 s later the Bluetooth cells are still on the pads");

    host = 1u;                      /* cable in again, the Mac sends its colours again */
    fake_intrusb = 0x04u;
    run_bt(100);
    plug();
    both_pages();
    check(lit_in(0u, 4u) == 4u && lit_in(8u, 4u) == 4u && pads_lit() == 8u, "X2 both links live again");
    bt_notify = 0u;                 /* Loopy on the iPhone writes CCCD 0, the link stays */
    run_bt(1000);
    check(lit_in(0u, 4u) == 4u && lit_in(8u, 4u) == 0u && pads_lit() == 4u,
          "X2 Bluetooth CCCD 0: the Bluetooth cells go dark, the USB cells stay on the pads");
    bt_notify = 1u;
    run_bt(50);
    both_pages();
    bt_up = bt_notify = 0u;         /* the Bluetooth link ends */
    run_bt(1000);
    check(lit_in(0u, 4u) == 4u && lit_in(8u, 4u) == 0u && pads_lit() == 4u,
          "X3 Bluetooth link end: the Bluetooth cells go dark, the USB cells stay on the pads");
    run_bt(9000);
    check(lit_in(0u, 4u) == 4u && pads_lit() == 4u, "X3 10 s later the USB cells are still on the pads");

    /* L. The same cell from both links: the last writer owns it. */
    bt_up = bt_notify = 1u;
    run_bt(50);
    both_pages();
    bt_note(0u, 0x31u);             /* cell 0: USB first (both_pages), then Bluetooth: Bluetooth owns it */
    usb_note(9u, 0x0Bu);            /* cell 9: Bluetooth first, then USB: USB owns it */
    run_bt(500);
    host = 0u;
    fake_intrusb = 0x01u;
    run_bt(1000);
    check(apc_led_velocity[0] == 0x31u && apc_led_velocity[9] == 0u && lit_in(1u, 3u) == 0u,
          "L1 USB end: cell 0 (Bluetooth wrote it last) stays, cell 9 (USB wrote it last) goes dark");
    host = 1u;
    fake_intrusb = 0x04u;
    run_bt(100);
    plug();
    usb_note(9u, 0x0Bu);
    run_bt(500);
    bt_up = bt_notify = 0u;
    run_bt(1000);
    check(apc_led_velocity[0] == 0u && apc_led_velocity[9] == 0x0Bu && lit_in(8u, 1u) == 0u && lit_in(10u, 2u) == 0u,
          "L2 Bluetooth end: cell 0 (Bluetooth wrote it last) goes dark, cell 9 (USB wrote it last) stays");

    /* C. Loopy quits on the Mac, the USB cable stays in (owner 01:49). */
    {
        static const uint32_t generic[4] = {   /* F0 47 7F 29 60 00 04 40 00 00 00 F7: APC40 mkII generic mode */
            4u | 0xF0u << 8 | 0x47u << 16 | 0x7Fu << 24, 4u | 0x29u << 8 | 0x60u << 16 | 0x00u << 24,
            4u | 0x04u << 8 | 0x40u << 16 | 0x00u << 24, 7u | 0x00u << 8 | 0x00u << 16 | 0xF7u << 24};
        uint32_t n, p[16];
        bt_up = bt_notify = 1u;
        usb_note(9u, 0u);                /* start with no cells: cell 9 of case L goes off */
        run_bt(1000);
        for (n = 0; n < 16u; n++)        /* USB: 16 cells, on channels 1-16 (steady, one-shot, pulse, blink) */
            p[n] = 9u | (0x90u | n) << 8 | (uint32_t)(n < 8u ? n : n + 8u) << 16 | (0x0Bu + n) << 24;
        usb_packet(p, 16u);
        run_bt(1000);
        for (n = 0; n < 16u; n++)        /* offs: note-off on even channels, note-on velocity 0 on odd ones */
            p[n] = n & 1u ? 9u | (0x90u | n) << 8 | (uint32_t)(n < 8u ? n : n + 8u) << 16
                          : 8u | (0x80u | n) << 8 | (uint32_t)(n < 8u ? n : n + 8u) << 16 | 0x40u << 24;
        usb_packet(p, 16u);
        run_bt(1000);
        check(lit_in(0u, 8u) == 0u && lit_in(16u, 8u) == 0u && pads_lit() == 0u,
              "C1 Loopy sends offs (note-off, note-on velocity 0, channels 1-16): all USB cells dark in 1 s");

        for (n = 0; n < 16u; n++)        /* the 256 ms hold (CLP-14): offs 50 ms after the colours */
            p[n] = 9u | 0x90u << 8 | (uint32_t)(n < 8u ? n : n + 8u) << 16 | 0x0Bu << 24;
        usb_packet(p, 16u);
        run_bt(50);
        for (n = 0; n < 16u; n++)
            p[n] = 8u | 0x80u << 8 | (uint32_t)(n < 8u ? n : n + 8u) << 16;
        usb_packet(p, 16u);
        run_bt(1000);
        check(lit_in(0u, 8u) == 0u && lit_in(16u, 8u) == 0u && pads_lit() == 0u,
              "C1 offs 50 ms after the colours (the 256 ms hold): all USB cells dark in 1 s");

        both_pages();
        usb_packet(generic, 4u);         /* Loopy gives the APC40 back: generic mode */
        run_bt(1000);
        check(lit_in(0u, 4u) == 0u && lit_in(8u, 4u) == 4u && pads_lit() == 4u,
              "C2 USB generic-mode SysEx (0x40): the USB cells go dark, the Bluetooth cells stay");
        usb_note(1u, 0x0Bu);
        run_bt(1000);
        check(apc_led_velocity[1] == 0x0Bu, "C2 USB feedback after the generic-mode SysEx shows");
    }

    /* D. Loopy closes on the iPhone. CLP-18: an off from Bluetooth always goes to the cell, also while notify is off. */
    bt_up = bt_notify = 1u;
    run_bt(50);
    bt_note(11u, 0x31u);
    bt_note(12u, 0x31u);
    run_bt(1000);
    (void)smk_apc_ble_event(8u | 0x80u << 8 | 12u << 16, 0u);    /* note-off while notify is off */
    (void)smk_apc_ble_event(9u | 0x9Eu << 8 | 11u << 16, 0u);    /* note-on velocity 0 (blink channel) */
    run_bt(1000);
    check(apc_led_velocity[12] == 0u && apc_led_velocity[11] == 0u, "D1 a Bluetooth off while notify is off clears the cell");
    (void)smk_apc_ble_event(9u | 0x90u << 8 | 13u << 16 | 0x31u << 24, 0u);
    run_bt(1000);
    check(apc_led_velocity[13] == 0u, "D1 a Bluetooth colour while notify is off still writes no cell");

    /* P. The PLAY (note 0x5B) and REC (note 0x5D) LED feedback follow the same rule as the cells (owner 02:00):
     *    the link that wrote it last owns it, and only the end of that link turns it off. */
    {
        static const uint32_t generic[4] = {
            4u | 0xF0u << 8 | 0x47u << 16 | 0x7Fu << 24, 4u | 0x29u << 8 | 0x60u << 16 | 0x00u << 24,
            4u | 0x04u << 8 | 0x40u << 16 | 0x00u << 24, 7u | 0x00u << 8 | 0x00u << 16 | 0xF7u << 24};
        bt_up = bt_notify = 1u;
        run_bt(50);
        usb_note(SMK_APC40_PLAY_NOTE, 127u);
        run_bt(100);
        check(apc_play_feedback == 1u, "P0 USB PLAY feedback on: the PLAY LED is on");
        host = 0u;
        fake_intrusb = 0x01u;
        run_bt(1000);
        check(apc_play_feedback == 0u, "P1 USB owns PLAY, USB cable out: the PLAY LED goes off");

        host = 1u;                       /* cable in again */
        fake_intrusb = 0x04u;
        run_bt(100);
        plug();
        bt_note(SMK_APC40_RECORD_NOTE, 127u);
        run_bt(100);
        host = 0u;
        fake_intrusb = 0x01u;
        run_bt(1000);
        check(apc_record_feedback == 1u, "P2 Bluetooth owns REC, USB cable out: the REC LED stays on");
        host = 1u;
        fake_intrusb = 0x04u;            /* a bus reset (smk_apc_host_lost) */
        run_bt(100);
        check(apc_record_feedback == 1u, "P2 Bluetooth owns REC, USB bus reset: the REC LED stays on");
        plug();

        usb_note(SMK_APC40_PLAY_NOTE, 127u);
        run_bt(100);
        bt_notify = 0u;                  /* Bluetooth CCCD 0 */
        run_bt(1000);
        check(apc_play_feedback == 1u && apc_record_feedback == 0u,
              "P3 USB owns PLAY, Bluetooth owns REC, Bluetooth CCCD 0: PLAY stays on, REC goes off");
        bt_notify = 1u;
        run_bt(50);
        bt_note(SMK_APC40_RECORD_NOTE, 127u);
        run_bt(100);
        bt_up = bt_notify = 0u;          /* the Bluetooth link ends */
        run_bt(1000);
        check(apc_play_feedback == 1u && apc_record_feedback == 0u,
              "P3 USB owns PLAY, Bluetooth owns REC, Bluetooth link end: PLAY stays on, REC goes off");

        bt_up = bt_notify = 1u;
        run_bt(50);
        usb_note(SMK_APC40_RECORD_NOTE, 127u);
        run_bt(100);
        usb_packet(generic, 4u);         /* generic mode on USB */
        run_bt(1000);
        check(apc_play_feedback == 0u && apc_record_feedback == 0u,
              "P4 USB owns PLAY and REC, USB generic-mode SysEx: both LEDs go off");
        bt_note(SMK_APC40_RECORD_NOTE, 127u);
        usb_note(SMK_APC40_PLAY_NOTE, 127u);
        bt_note(SMK_APC40_PLAY_NOTE, 127u);   /* PLAY: USB first, then Bluetooth: Bluetooth owns it */
        run_bt(100);
        usb_packet(generic, 4u);
        run_bt(1000);
        check(apc_play_feedback == 1u && apc_record_feedback == 1u,
              "P4 Bluetooth owns PLAY (last writer) and REC, USB generic-mode SysEx: both LEDs stay on");
        (void)smk_apc_ble_event(8u | 0x80u << 8 | SMK_APC40_RECORD_NOTE << 16, 0u);   /* an off while notify is off */
        run_bt(100);
        check(apc_record_feedback == 0u, "P5 a Bluetooth REC off while notify is off turns the REC LED off");
    }

    printf("%s %d fail\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
