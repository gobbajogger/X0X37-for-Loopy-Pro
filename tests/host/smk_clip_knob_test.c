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
#define FM1_IRQ_TIMER5 63u
static uint32_t fm1_irq_mask_save(unsigned n) { (void)n; return 0; }
static void fm1_irq_mask_restore(unsigned n, uint32_t s) { (void)n; (void)s; }
#define BOARD_SMK37 1
#define X0X_BLE 0
#define SMK_BIG
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
#include "src/usb.c"
void smc_mmc_host(uint8_t cmd, uint8_t src) { (void)cmd; (void)src; }
#undef BOARD_SMK37
#define X0X_HOST 1
#include "app/plat.h"
typedef struct {                    /* the clip keys fields of settings_t (app/x0x.h) */
    uint8_t ck_ch, ck_root, ck_scale, ck_snap, ck_chord, ck_inv, ck_ord, ck_rate, ck_oct, ck_gate, ck_x1, ck_x2;
    uint8_t autosave_off;
} settings_t;
static struct { settings_t set; } proj;
static struct { uint16_t bpm; uint8_t playing, req_start, req_cont, req_stop; } seq;
static uint8_t project_busy;
static int ui_project_busy(void) { return project_busy; }
/* This clip/MIDI fixture has no active boot capture or maintenance lease. */
int ui_boot_flash_busy(void) { return 0; }
#include "fm1_opt.h"
#define SMK_KEY_F3 5u
#define SMK_BTN_KNOB_BANK 0
#define SMK_BTN_FADER_BANK 1
#define SMK_BTN_LEFT 2
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
static struct { uint8_t overlay, help, rec, last_autosave, save_failed; uint32_t act_t, saved_t, dirty; } ui;
static void mark_dirty(void) { ui.dirty++; }
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
/* ck_a.inc calls production SMC clip popup helpers; this fixture owns MIDI/CC
 * and chord-screen behaviour, not presentation — same stub class as
 * smc_clip_visibility_test.c (#453), different harness. */
static void smc_ui_clip_show(int n, const char *name, const char *value)
{
    (void)n;
    (void)name;
    (void)value;
}
static void smc_ui_cancel(void) {}
#include "ck_a.inc"
#include "ck_b.inc"
static void ck_page_draw(void) {}
static uint8_t save_wait;
static uint32_t save_calls;
static uint8_t perf_testing(void) { return 0u; }
static uint8_t perf_save_busy(void) { return 0u; }
static int project_save(void) { save_calls++; return 0; }
static void say(const char *s, uint32_t t) { (void)s; (void)t; }
static void save_project(void) { save_wait = 0u; project_save(); }
#define BOARD_SMK37 1
#include "autosave.inc"
#undef BOARD_SMK37

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

/* K9: tap a stage button (SCALE B_SEL, CHORD B_ENV, ARP B_ARP): down 50 ms, up, then 400 ms */
static void tap(uint32_t role)
{
    fm1_in.buttons |= 1u << panel.btn[role];
    run(50);
    fm1_in.buttons &= ~(1u << panel.btn[role]);
    run(400);
}
/* turn knob role r (0 = knob 2) by d detents; check that field *fld goes from its value to want, and that no
 * knob CC goes out (want_cc 0) or that the CC goes out and the field stays (want_cc 1) */
static void knob_edit(const char *what, uint32_t r, int32_t d, const uint8_t *fld, uint32_t want, int want_cc)
{
    unsigned u0 = fake_tx_n, b0 = ble_tx_n, k;
    uint32_t was = *fld;
    fm1_in.enc_steps[r] = (int16_t)d;
    run(20);
    k = cc_in(fake_tx + u0, fake_tx_n - u0, 48u + r) + cc_in(ble_tx + b0, ble_tx_n - b0, 48u + r);
    {
        int ok = *fld == want && (want_cc ? k != 0u : k == 0u);
        printf("%s %s (field %u -> %u, want %u; CC %u)\n", ok ? "PASS" : "FAIL", what, was, *fld, want, k);
        fails += !ok;
    }
    if (fake_tx_n > 200u) fake_tx_n = 0;
    if (ble_tx_n > 200u) ble_tx_n = 0;
}

/* K10: press keybed key k for 20 ms; return the note-ons on the CLIP KEYS channel (ch 2) on Bluetooth, the
 * lowest note in *lo */
static unsigned key_notes(uint32_t k, uint32_t *lo)
{
    unsigned i, n = 0;
    ble_tx_n = 0;
    fm1_in.keys |= (uint64_t)1u << k;
    run(20);
    *lo = 128u;
    for (i = 0; i < ble_tx_n; i++)
        if (((ble_tx[i] >> 8) & 0xFFu) == 0x91u && (ble_tx[i] >> 24)) {
            n++;
            if (((ble_tx[i] >> 16) & 0x7Fu) < *lo)
                *lo = (ble_tx[i] >> 16) & 0x7Fu;
        }
    fm1_in.keys &= ~((uint64_t)1u << k);
    run(50);
    ble_tx_n = 0;
    return n;
}
static void fader_bank(void)        /* FADER BANK: a pinned page closes */
{
    fm1_in.buttons |= 1u << SMK_BTN_FADER_BANK;
    run(30);
    fm1_in.buttons &= ~(1u << SMK_BTN_FADER_BANK);
    run(30);
}
static void check(const char *what, int ok)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    fails += !ok;
}
#define B_SCALE_ROLE B_SEL
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
    /* K9: stage buttons show settings without switching the stage on. */
    tap(B_ARP);
    check("K9 ARP opens on a press and stays off", (ck.pg & 3u) == 3u && !(ck.on & CK_M_ARP));
    check("K9 ON is selected", (ck.pg >> 4) == CK_E_ON);
    fm1_in.enc_steps[1] = 1;           /* knob 3: stage ON */
    run(20);
    check("K9 knob 3 enables ARP", (ck.on & CK_M_ARP) != 0u);
    fm1_in.enc_steps[0] = 1;           /* knob 2: next field (ORDER) */
    run(20);
    check("K9 knob 2 selects ORDER", (ck.pg >> 4) == CK_E_ORD);
    knob_edit("K9 knob 3 edits ORDER", 1u, 1, &proj.set.ck_ord, 1u, 0);
    fm1_in.buttons |= 1u << (SMK_BTN_LEFT + 1u);
    run(20);
    check("K9 > steps selected ORDER", proj.set.ck_ord == 2u);
    fm1_in.buttons &= ~(1u << (SMK_BTN_LEFT + 1u));
    run(20);
    fm1_in.buttons |= 1u << SMK_BTN_LEFT;
    run(20);
    check("K9 < steps selected ORDER", proj.set.ck_ord == 1u);
    fm1_in.buttons &= ~(1u << SMK_BTN_LEFT);
    run(20);
    tap(B_ARP);
    check("K9 second press closes page and keeps ARP on", !(ck.pg & 3u) && (ck.on & CK_M_ARP));
    tap(B_ARP);
    fm1_in.enc_steps[1] = -1;         /* ARP off so a key sounds at once */
    run(20);
    tap(B_ARP);
    tap(B_SCALE_ROLE);
    check("K9 SCALE opens without enabling", (ck.pg & 3u) == 1u && !(ck.on & CK_M_SCALE));
    fm1_in.enc_steps[1] = 1;
    run(20);
    check("K9 SCALE ON can be edited", (ck.on & CK_M_SCALE) != 0u);
    { unsigned n; uint32_t lo;
      n = key_notes(1u, &lo);
      check("K9 keybed still plays while SCALE page is open", n == 1u);
    }
    tap(B_ENV);
    check("K9 CHORD replaces SCALE page, stages unchanged", (ck.pg & 3u) == 2u && !(ck.on & CK_M_CHORD) && (ck.on & CK_M_SCALE));
    tap(B_ENV);
    check("K9 CHORD page closes", !(ck.pg & 3u));
    knob("K9 no page: CC controls resume", 1);
    /* A clip performance must not enter flash erase while the raw outer keybed is held. The first
     * check runs before smk_ck_poll, as ui_frame does on display frames. */
    save_calls = 0u;
    ui.dirty = 1u;
    ui.saved_t = 0u;
    run(AUTOSAVE_QUIET + 1u);
    fm1_in.keys |= (uint64_t)1u << 36;
    autosave();
    check("K11 outer C6 blocks autosave before key polling", save_calls == 0u);
    run(AUTOSAVE_QUIET + 1u);
    autosave();
    check("K11 held outer C6 keeps autosave quiet", save_calls == 0u && ui.act_t == fm1_ms);
    fm1_in.keys &= ~((uint64_t)1u << 36);
    run(1);
    autosave();
    check("K11 release starts the quiet interval", save_calls == 0u && ui.act_t == fm1_ms);
    run(AUTOSAVE_QUIET);
    autosave();
    check("K11 autosave resumes after the quiet interval", save_calls == 1u);
    ui.dirty = 1u;
    ui.saved_t = 0u;
    run(AUTOSAVE_QUIET + 1u);
    fm1_in.enc_steps[0] = 1;                         /* CC 48, with no stage page open */
    run(1);
    autosave();
    check("K11 clip CC turn starts the quiet interval", save_calls == 1u && ui.act_t == fm1_ms);
    run(AUTOSAVE_QUIET);
    autosave();
    check("K11 autosave resumes after the CC turn", save_calls == 2u);
    fm1_in.keys |= (uint64_t)1u << 36;
    save_wait = 1u;                                  /* manual SAVE requested during playback */
    autosave();
    check("K11 queued SAVE waits during a held clip note", save_calls == 2u && save_wait);
    run(AUTOSAVE_QUIET + 1u);
    autosave();
    check("K11 queued SAVE remains pending through the held note", save_calls == 2u && save_wait);
    fm1_in.keys &= ~((uint64_t)1u << 36);
    run(1);
    autosave();
    check("K11 queued SAVE waits after key release", save_calls == 2u && save_wait);
    run(AUTOSAVE_QUIET);
    fm1_in.enc_steps[0] = 1;                         /* new CC turn, before the next clip poll */
    autosave();
    check("K11 queued SAVE waits for a pending clip CC turn", save_calls == 2u && save_wait);
    run(1);
    autosave();
    check("K11 queued SAVE waits after the CC turn", save_calls == 2u && save_wait);
    run(AUTOSAVE_QUIET);
    autosave();
    check("K11 queued SAVE runs once performance is quiet", save_calls == 3u && !save_wait);
    {
        settings_t saved = proj.set;
        unsigned u0 = fake_tx_n, b0 = ble_tx_n;
        uint8_t page = ck.pg, on = ck.on;
        project_busy = 1;
        ck_start_req = 1;
        fm1_in.keys = (uint64_t)1u << 36;
        fm1_in.buttons = 1u << panel.btn[B_ARP];
        fm1_in.enc_steps[0] = 3;
        run(20);
        check("K12 project hold suppresses key, stage and knob MIDI/edits",
              !memcmp(&saved, &proj.set, sizeof saved) && ck.pg == page && ck.on == on &&
              fake_tx_n == u0 && ble_tx_n == b0);
        check("K12 project hold consumes raw key edges and stale start",
              ck.keys[1] == 16u && ck.btn_prev == 0x7Fu && !ck_start_req && ck.ext_seen == (uint8_t)apc_clock_count);
        save_wait = 1;
        autosave();
        check("K12 project hold blocks queued SAVE", save_calls == 3u && save_wait);
        /* The production UI drains encoders separately while held; this fixture only polls clip keys. */
        for (i = 0; i < NE; i++)
            (void)fm1_enc_take(panel.enc[i]);
        project_busy = 0;
        run(20);
        check("K12 held key and stage button do not retrigger on release",
              !ck.snd[0] && !ck.snd[1] && ck.pg == page && ck.on == on);
        fm1_in.keys = fm1_in.buttons = 0;
        run(20);
        knob("K12 fresh knob turns resume after project hold", 1);
    }
    printf("%s %d fail\n", fails ? "FAIL" : "PASS", fails);
    return fails != 0;
}
