/* SPDX-License-Identifier: GPL-3.0-only */
/* CLP-50: in clip mode, one PLAY press must give Loopy Pro one APC PLAY note on (90 5B 7F), and one
 * release one note off (80 5B), in total on USB and Bluetooth. Loopy Pro uses the note on as a
 * toggle. Two copies start and then stop the transport (owner test a73 FAIL, INLOG 9a733fe 22:48:46).
 * It runs the REAL main_fm1.c code: apc_out, apc_note and smk_clip_transport_button, cut out by
 * tests/host/smk_clip_play_ports_test.sh. The stubs below have the same send conditions as:
 *   usb.c midi_out_event: the packet goes into the USB queue only when the host configured USB.
 *   ble.c ble_out: the packet goes into the Bluetooth queue only when the BLE host runs and the
 *   central turned notify on (bleh_notifying). With notify off, ble_out drops the packet. */
#include <stdint.h>
#include <stdio.h>
#define X0X_BLE 1
#include "app/plat.h"
#include "app/smk_apc40.h"

static uint8_t usb_config, ble_notify;      /* the link state of each case */
static unsigned usb_on, usb_off, ble_on, ble_off;
static void count(uint32_t pkt, unsigned *on, unsigned *off)
{
    uint32_t st = (pkt >> 8) & 0xFFu, d1 = (pkt >> 16) & 0x7Fu, d2 = (pkt >> 24) & 0x7Fu;
    if (st == 0x90u && d1 == SMK_APC40_PLAY_NOTE && d2 == 0x7Fu)
        ++*on;
    if (st == 0x80u && d1 == SMK_APC40_PLAY_NOTE)
        ++*off;
}
static void midi_out_event(uint32_t pkt) { if (usb_config) count(pkt, &usb_on, &usb_off); }
static void ble_out(uint32_t pkt) { if (ble_notify) count(pkt, &ble_on, &ble_off); }
static uint32_t irq_save(void) { return 0u; }
static void irq_restore(uint32_t f) { (void)f; }

static volatile uint8_t clip_grid_mode;
static volatile uint8_t apc_play_feedback, apc_record_feedback;
#include "apc_out.inc"
#include "clip_play.inc"

static int fails;
static void reset(uint8_t usb, uint8_t ble)
{
    usb_config = usb;
    ble_notify = ble;
    usb_on = usb_off = ble_on = ble_off = 0u;
}
static void check(int ok, const char *what)
{
    printf("%s %s (note on: USB %u, Bluetooth %u; note off: USB %u, Bluetooth %u)\n", ok ? "PASS" : "FAIL",
           what, usb_on, ble_on, usb_off, ble_off);
    fails += !ok;
}

int main(void)
{
    clip_grid_mode = 1u;

    /* A. Loopy Pro on USB and on Bluetooth (notify on): the owner's set-up in the INLOG. */
    reset(1u, 1u);
    (void)smk_clip_transport_button(B_PLAY, 1u);
    check(usb_on + ble_on == 1u, "A1 one PLAY press: one note on 90 5B 7F on USB and Bluetooth together");
    (void)smk_clip_transport_button(B_PLAY, 0u);
    check(usb_off + ble_off == 1u, "A2 one PLAY release: one note off 80 5B on USB and Bluetooth together");

    /* B. No regression: USB only (Bluetooth not connected or notify off). */
    reset(1u, 0u);
    (void)smk_clip_transport_button(B_PLAY, 1u);
    (void)smk_clip_transport_button(B_PLAY, 0u);
    check(usb_on == 1u && usb_off == 1u, "B  USB only: the press and the release go out on USB");

    /* C. No regression: Bluetooth only (USB not configured, for example power from a charger). */
    reset(0u, 1u);
    (void)smk_clip_transport_button(B_PLAY, 1u);
    (void)smk_clip_transport_button(B_PLAY, 0u);
    check(ble_on == 1u && ble_off == 1u, "C  Bluetooth only: the press and the release go out on Bluetooth");

    printf("%s %d fail\n", fails ? "FAIL" : "PASS", fails);
    return fails ? 1 : 0;
}
