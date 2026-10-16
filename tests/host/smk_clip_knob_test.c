/* SPDX-License-Identifier: GPL-3.0-only */
/* Owner report 2026-10-17 02:49 on c6bf44a (rel): in clip mode the knobs do nothing. On f9bcc46 they sent
 * to Loopy. In clip mode with no menu, knobs 2-8 send CC 48-54 (relative) to USB and Bluetooth
 * (main_fm1.c ck_controls, apc_out; BEHAVIOR.md).
 * It runs the REAL clip keybed code (smk_ck_poll, ck_controls), the REAL clip feedback code (apc_out, the
 * CLP-18 link code) and the REAL usb.c (usb_poll at 2 kHz, ep1_rx, ep1_tx) on a model of the SIE registers
 * (smk_clip_knob_fake_usb.h). The script cuts the code out of main_fm1.c and lcd.c.
 * The host sends what Loopy sends: the APC40 introduction (modes 0x41 and 0x40), the clip colours, and the
 * link events (Mac asleep, cable out, Bluetooth drop). After each event a knob turn must send its CC. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define BOARD_SMK37 1
#define X0X_BLE 0
#define SMK_BIG
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
#include "src/usb.c"
#undef BOARD_SMK37
#define X0X_HOST 1
#include "app/plat.h"
typedef struct {                    /* the clip keys fields of settings_t (app/x0x.h) */
    uint8_t ck_ch, ck_root, ck_scale, ck_snap, ck_chord, ck_inv, ck_ord, ck_rate, ck_oct, ck_gate, ck_x1, ck_x2;
} settings_t;
static struct { settings_t set; } proj;
static struct { uint16_t bpm; } seq;
#include "fm1_opt.h"
#define SMK_KEY_F3 5u
#define SMK_BTN_FADER_BANK 1
static volatile uint32_t fm1_ms;
uint32_t plat_ms(void) { return fm1_ms; }
static uint8_t bt_up, bt_notify;   /* the Bluetooth link (ble.c gives it each poll) */
int plat_ble_state(void) { return bt_up ? PLAT_BLE_CONNECTED : 0; }
#define C_BLACK 0
#define C_WHITE 0xFFFF
#define C_DIM 0x4208
#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
static void cv_begin(uint32_t w, uint32_t h, uint16_t c) { (void)w; (void)h; (void)c; }
static void cv_band(int32_t a, int32_t b) { (void)a; (void)b; }
static void cv_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c) { (void)x; (void)y; (void)w; (void)h; (void)c; }
enum { O_NONE, O_LIST, O_ASK };
static struct { uint8_t overlay, help; uint32_t act_t; } ui;
static struct { uint8_t btn[NB], enc[NE]; int8_t dir[NE]; } panel;
static volatile struct {
    uint32_t pnotes, buttons;
    uint64_t keys;
    uint8_t key_vel[40];
    int16_t enc_steps[8];
} fm1_in;
static int32_t fm1_enc_take(uint32_t e) { int32_t s = fm1_in.enc_steps[e]; fm1_in.enc_steps[e] = 0; return s; }
static const int8_t KEY_WHITE[NKEYS] = {0, -1, 1, -1, 2, -1, 3, 4, -1, 5, -1, 6, 7, -1, 8, -1, 9, -1, 10, 11, -1, 12, -1, 13, 14, -1, 15};
static void ui_say(const char *a, const char *b) { (void)a; (void)b; }
static char *put_s(char *d, const char *s) { while (*s) *d++ = *s++; *d = 0; return d; }
static char *put_i(char *d, int v) { return d + sprintf(d, "%d", v); }
static void lcd_sync(void) {}
static void fm1_padled_send(const void *p, uint32_t n) { (void)p; (void)n; }
static void fm1_lcd_wait(void) {}
static void fm1_padled_done(void) {}
static uint8_t smk_pad_grid = 1;
#include "padled.inc"
#include "app/smk_apc40.h"
#include "app/apc40_lcd565.h"
#if __has_include("app/apc40_lcd_idle.h")
#include "app/apc40_lcd_idle.h"
#include "app/smk_clip_colour.h"
#endif
static uint32_t ble_tx[256];        /* what goes to Bluetooth (ble.c ble_out) */
static unsigned ble_tx_n;
static void ble_out(uint32_t p) { if (ble_tx_n < 256u) ble_tx[ble_tx_n++] = p; }
static uint32_t irq_save(void) { return 0u; }
static void irq_restore(uint32_t f) { (void)f; }
#undef X0X_BLE
#define X0X_BLE 1
#include "apc_a.inc"
#include "app/smk_clip_keys.h"
static volatile uint8_t ck_start_req;
void smk_ck_midi_stop(void);
void smk_inlog(uint32_t src, uint32_t pkt) { (void)src; (void)pkt; }
#include "apc_b.inc"
#include "ck_a.inc"
#include "ck_b.inc"
static void ck_page_draw(void) {}

static uint8_t host;                /* 1: a host is on the cable and awake (SOF each 1 ms) */
static void run(uint32_t ms)        /* usb_poll at 2 kHz, ble.c link poll and the main loop smk_ck_poll each 1 ms */
{
    while (ms--) {
        fm1_ms++;
        if (host) {
            fake_frame = (uint16_t)((fake_frame + 1u) & 0x7FFu);
            fake_sof = 1u;
        }
        usb_poll();
        smk_apc_ble_link(bt_up, bt_notify);
        usb_poll();
        smk_ck_poll();
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
static void usb_intro(uint32_t mode)   /* Loopy: F0 47 7F 29 60 00 04 mode 01 00 00 F7 (USB-MIDI CIN 4, 4, 4, 7) */
{
    uint32_t p[4] = {0x7F47F004u, 0x00602904u, 0x01000404u | mode << 24, 0xF7000007u};
    usb_packet(p, 4u);
    run(20);
}
static void usb_inquiry(void)       /* Universal Device Inquiry F0 7E 7F 06 01 F7 */
{
    uint32_t p[2] = {0x7F7EF004u, 0xF7010606u};
    usb_packet(p, 2u);
    run(20);
}
static void plug(void)              /* enumerated: usb.c SET_CONFIGURATION 1, then Loopy's handshake */
{
    host = 1u;
    usb.up = 1u;
    usb.config = 1u;
    ep1_config();
    run(200);
    usb_inquiry();
    usb_intro(0x41u);
}
static void colours(void)           /* Loopy's clip colours: cells 0-3 over USB, cells 8-11 over Bluetooth */
{
    uint32_t n, p[4];
    for (n = 0; n < 4u; n++) {
        p[n] = 9u | 0x90u << 8 | n << 16 | 0x0Bu << 24;
        if (bt_up)
            (void)smk_apc_ble_event(9u | 0x90u << 8 | (8u + n) << 16 | 0x31u << 24, bt_notify);
    }
    usb_packet(p, 4u);
    run(500);
}
static unsigned cc_in(const uint32_t *q, unsigned n, uint32_t cc)   /* CC cc on channel 1 in a packet list */
{
    unsigned i, k = 0;
    for (i = 0; i < n; i++)
        k += ((q[i] >> 8) & 0xFFu) == 0xB0u && ((q[i] >> 16) & 0x7Fu) == cc;
    return k;
}
static int fails;
static void knob(const char *what, int want_usb)   /* turn knob 2 (role 0) 3 detents, then role 3: CC 48, CC 51 */
{
    unsigned u0 = fake_tx_n, b0 = ble_tx_n, u48, u51, b48, b51;
    fm1_in.enc_steps[0] = 3;
    run(20);
    fm1_in.enc_steps[3] = -2;
    run(20);
    u48 = cc_in(fake_tx + u0, fake_tx_n - u0, 48u);
    u51 = cc_in(fake_tx + u0, fake_tx_n - u0, 51u);
    b48 = cc_in(ble_tx + b0, ble_tx_n - b0, 48u);
    b51 = cc_in(ble_tx + b0, ble_tx_n - b0, 51u);
    {
        int ok = b48 == 1u && b51 == 1u && (want_usb < 0 || (want_usb ? u48 == 1u && u51 == 1u : u48 + u51 == 0u)) &&
                 smk_ck_knobs_owned();
        printf("%s %s (USB CC48 %u CC51 %u, BT CC48 %u CC51 %u, clip %u owned %u)\n", ok ? "PASS" : "FAIL", what,
               u48, u51, b48, b51, clip_grid_mode, smk_ck_knobs_owned());
        fails += !ok;
    }
    if (fake_tx_n > 200u) fake_tx_n = 0;
    if (ble_tx_n > 200u) ble_tx_n = 0;
}

int main(void)
{
    uint32_t i;
    for (i = 0; i < NE; i++) {
        panel.enc[i] = (uint8_t)i;
        panel.dir[i] = 1;
    }
    for (i = 0; i < NB; i++)
        panel.btn[i] = (uint8_t)(4u + i);   /* (no button is down) */
    proj.set.ck_ch = 2u;
    clip_grid_mode = 1u;
    run(100);
    knob("K0 clip mode, no host: the knobs send on Bluetooth", 0);
    plug();
    knob("K1 Loopy on the Mac (USB, mode 0x41): the knobs send CC 48 and CC 51", 1);
    bt_up = bt_notify = 1u;
    run(50);
    colours();
    knob("K2 both links up, clip colours on both: the knobs send", 1);
    run(5000);
    knob("K2 5 s later: the knobs send", 1);
    usb_intro(0x42u);
    knob("K3 the introduction with mode 0x42: the knobs send", 1);
    usb_intro(0x40u);
    knob("K4 the introduction with mode 0x40 (generic): the knobs send", 1);
    usb_intro(0x41u);
    colours();
    bt_notify = 0u;
    run(1000);
    knob("K5 Bluetooth notify off (Loopy closed on the iPhone): the knobs send", 1);
    bt_up = 0u;
    run(1000);
    knob("K5 Bluetooth link end: the knobs send", 1);
    bt_up = bt_notify = 1u;
    run(50);
    colours();
    host = 0u;                      /* the Mac sleeps: no SOF, the suspend interrupt */
    fake_intrusb = 0x01u;
    run(1000);
    knob("K6 the Mac sleeps (USB suspend): the knobs send on Bluetooth", -1);   /* (USB: usb.c queues while configured) */
    host = 1u;                      /* the Mac wakes: SOF again (resume) */
    fake_intrusb = 0x02u;
    run(100);
    knob("K6 the Mac wakes (resume): the knobs send on USB again", 1);
    host = 0u;                      /* the cable out, then in again: bus reset, a new enumeration */
    fake_intrusb = 0x01u;
    run(1000);
    host = 1u;
    fake_intrusb = 0x04u;
    run(100);
    plug();
    colours();
    knob("K7 the cable in again (bus reset, enumeration): the knobs send", 1);
    run(10000);
    knob("K7 10 s later: the knobs send", 1);
    /* K8 (the control: the test sees a knob that sends nothing). A menu or a help card on the screen takes
     * the knobs (smk_ck_knobs_owned 0): no CC. */
    {
        unsigned u0 = fake_tx_n, b0 = ble_tx_n, k;
        ui.overlay = O_LIST;
        fm1_in.enc_steps[0] = 3;
        run(20);
        ui.overlay = O_NONE;
        ui.help = 1u;
        fm1_in.enc_steps[3] = 3;
        run(20);
        ui.help = 0u;
        k = cc_in(fake_tx + u0, fake_tx_n - u0, 48u) + cc_in(fake_tx + u0, fake_tx_n - u0, 51u) +
            cc_in(ble_tx + b0, ble_tx_n - b0, 48u) + cc_in(ble_tx + b0, ble_tx_n - b0, 51u);
        printf("%s K8 a menu or a help card on the screen: the knobs send no CC (%u CC)\n", k ? "FAIL" : "PASS", k);
        fails += k != 0u;
        fm1_in.enc_steps[0] = fm1_in.enc_steps[3] = 0;
    }
    knob("K8 the menu and the help card closed: the knobs send again", 1);
    printf("%s %d fail\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
