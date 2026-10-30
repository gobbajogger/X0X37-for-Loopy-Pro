/* SPDX-License-Identifier: GPL-3.0-only */
/* The parts of the Bluetooth MIDI glue (src/ble.c) that touch no hardware, so the host tests
 * (tests/ble_test.c) compile them as they are:
 *   ble_img_check()    is the flash block a controller image this app can start?
 *   ble_usb_of_msg()   a BLE-MIDI message as a USB-MIDI event packet (the queue usb.c reads)
 *   ble_msg_of_usb()   a USB-MIDI event packet as the bytes of one MIDI message
 *   ble_addr_of()      the random static address and the name from the factory record
 *   ble_guard_*()      the start guard: a controller start that failed, crashed or reset the unit
 *                      is not tried again at the next power-on (STATUS FAILED, BEHAVIOR.md BLE-12) */
#pragma once
#include <stdint.h>
#include "x0x_ble_abi.h"
#include "ble_midi_host.h"

enum { BLE_IMG_OK = 0, BLE_IMG_NONE = 1, BLE_IMG_BAD = 2 };

/* img: the block as the CPU reads it (XIP), size bytes of it readable */
static int ble_img_check(const uint8_t *img, uint32_t size, uint32_t xip_base)
{
    const xb_header_t *h = (const xb_header_t *)(const void *)img;
    static const uint8_t zero4[4] = {0, 0, 0, 0};
    uint32_t crc, end;
    if (h->magic != XB_HDR_MAGIC)
        return BLE_IMG_NONE;                    /* an empty block (0xFF) or anything else */
    if (h->hdr_version != XB_HDR_VERSION || h->hdr_size != sizeof(xb_header_t) || h->abi != XB_ABI)
        return BLE_IMG_BAD;
    if (h->image_size < sizeof(xb_header_t) || h->image_size > size)
        return BLE_IMG_BAD;
    crc = xb_crc32(0, img, 16u);
    crc = xb_crc32(crc, zero4, 4u);
    crc = xb_crc32(crc, img + 20u, h->image_size - 20u);
    if (crc != h->crc32)
        return BLE_IMG_BAD;
    end = xip_base + h->image_size;
    if (h->init < xip_base + sizeof(xb_header_t) || h->init >= end ||
        h->poll < xip_base + sizeof(xb_header_t) || h->poll >= end)
        return BLE_IMG_BAD;
    if (h->ram_lo < XB_RAM_LO || h->ram_hi > XB_RAM_HI || h->ram_lo >= h->ram_hi)
        return BLE_IMG_BAD;
    if (h->isr_stack_lo < h->ram_lo || h->isr_stack_hi > h->ram_hi || h->isr_stack_lo >= h->isr_stack_hi ||
        (h->isr_stack_hi & 7u))
        return BLE_IMG_BAD;
    /* never an interrupt X0X uses on core 0: the exception, ALNK0, SPI1, UART1, SARADC, SPI2, 62, TIMER5 */
    if ((h->irq_lo & ((1u << 1) | (1u << 11) | (1u << 16) | (1u << 20) | (1u << 24))) ||
        (h->irq_hi & ((1u << (37 - 32)) | (1u << (62 - 32)) | (1u << (63 - 32)))))
        return BLE_IMG_BAD;
    return BLE_IMG_OK;
}

/* a BLE-MIDI message as a USB-MIDI event packet, cable 0 (0 = none) */
static uint32_t ble_usb_of_msg(const bleh_msg_t *m)
{
    uint32_t st = m->b[0], cin;
    if (st < 0x80u)
        return 0;
    if (st < 0xF0u)
        cin = st >> 4;                          /* 8..E: the channel messages */
    else if (st >= 0xF8u || st == 0xF6u)
        cin = st == 0xF6u ? 5u : 0xFu;          /* one byte */
    else if (st == 0xF1u || st == 0xF3u)
        cin = 2u;
    else if (st == 0xF2u)
        cin = 3u;
    else
        return 0;                               /* F0 F7 (SysEx comes another way), F4 F5 */
    return cin | st << 8 | (uint32_t)m->b[1] << 16 | (uint32_t)m->b[2] << 24;
}

/* a USB-MIDI event packet as one MIDI message: its length (0 = not one BLE-MIDI sends this way) */
static uint32_t ble_msg_of_usb(uint32_t pkt, uint8_t b[3])
{
    uint32_t cin = pkt & 0xFu, st = (pkt >> 8) & 0xFFu, n;
    b[0] = (uint8_t)st;
    b[1] = (uint8_t)((pkt >> 16) & 0x7Fu);
    b[2] = (uint8_t)((pkt >> 24) & 0x7Fu);
    if (cin >= 8u) {
        if (cin == 0xFu)
            return st >= 0xF8u ? 1u : 0u;       /* a single byte: real time only */
        if ((st >> 4) != cin)
            return 0;
        n = (cin == 0xCu || cin == 0xDu) ? 2u : 3u;
    } else if (cin == 2u && (st == 0xF1u || st == 0xF3u)) {
        n = 2u;
    } else if (cin == 3u && st == 0xF2u) {
        n = 3u;
    } else if (cin == 5u && st == 0xF6u) {
        n = 1u;
    } else {
        return 0;                               /* SysEx and anything else: not mirrored */
    }
    return n;
}

/* the address and the name "X0X-XXXX" from a factory record (the same on every boot of a unit,
 * different between units): a random static address, top two bits 11 */
static void ble_addr_of(const uint8_t *rec, uint32_t n, uint8_t addr[6], char name[9])
{
    /* Same 16 digits ui.c and main_fm1.c already index. One literal in this TU. */
    uint32_t a = xb_crc32(0, rec, n), b = xb_crc32(0x5A5A5A5Au, rec, n);
    addr[0] = (uint8_t)a;
    addr[1] = (uint8_t)(a >> 8);
    addr[2] = (uint8_t)(a >> 16);
    addr[3] = (uint8_t)(a >> 24);
    addr[4] = (uint8_t)b;
    addr[5] = (uint8_t)((b >> 8) | 0xC0u);
    name[0] = 'X'; name[1] = '0'; name[2] = 'X'; name[3] = '-';
    name[4] = "0123456789ABCDEF"[addr[1] >> 4]; name[5] = "0123456789ABCDEF"[addr[1] & 15u];
    name[6] = "0123456789ABCDEF"[addr[0] >> 4]; name[7] = "0123456789ABCDEF"[addr[0] & 15u];
    name[8] = 0;
}

/* ---- the start guard (src/ble.c keeps it in .noinit: it survives a reset, not a power-off) ----
 * Before the controller's init the app sets trying. A failed start (it did not come back, a wrong
 * job, not READY) sets blocked at once. A start that resets or freezes the unit leaves trying set:
 * the next boot sees it and sets blocked. While blocked, the controller does not start (STATUS
 * FAILED). BLUETOOTH OFF in GLO clears blocked: the next power-on tries again. trying ends when the
 * controller has run BLE_GUARD_OK_MS after its start. */
#define BLE_GUARD_MAGIC 0x44524742u              /* "BGRD" */
#define BLE_GUARD_OK_MS 10000u
enum { BLE_WHY_NONE = 0, BLE_WHY_TIMEOUT = 1, BLE_WHY_WRONG_JOB = 2, BLE_WHY_NOT_READY = 3, BLE_WHY_RESET = 4,
       BLE_WHY_NO_TICK = 5 };
typedef struct {
    uint32_t magic, trying, blocked, why, fails;
    uint32_t start_ms, init_ms, mb_state, mb_err;  /* the last start: its time, how long init took, the mailbox */
    uint32_t dbg[6];                               /* the image's own record of the last start (xb_mbox_t dbg) */
    uint32_t reset;                                /* BLE_WHY_RESET: P3 reset source | RST_SRC << 8 of that reset */
    uint32_t sink_n, sink_at, sink_val;            /* the RAM sink after the start: words changed, the first */
    uint32_t wait_end;                             /* fm1_c1_wait_end of the start (fm1_cpu1.h) */
    uint32_t xosc_pre, xosc_post;                  /* WLA_CON12 (crystal trim) before the start and after init */
    uint32_t wait_ms;                              /* fm1_c1_wait_ms: core 0's last look in the wait, ms */
    uint32_t seen[4];                              /* the image's dbg[0..3] at that look (fm1_c1_wait_seen) */
    uint32_t code_bad, code_sum;                   /* RAM code 0x01C00400..XB_RAM_LO changed: 0xBAD00000 | ms, the sum */
} ble_guard_t;

/* power-on: 1 = start the controller now */
static int ble_guard_boot(ble_guard_t *g, int img_ok, int on)
{
    uint32_t i;
    if (g->magic != BLE_GUARD_MAGIC) {
        uint8_t *z = (uint8_t *)g;
        for (i = 0; i < sizeof *g; i++)
            z[i] = 0;
        g->magic = BLE_GUARD_MAGIC;
    }
    if (g->trying) {                             /* the last start did not finish: a reset or a hang */
        g->trying = 0;
        g->blocked = 1;
        g->why = BLE_WHY_RESET;
        g->fails++;
    }
    if (!img_ok || !on || g->blocked)
        return 0;
    g->trying = 1;
    return 1;
}
/* after the controller's init: rc (0 from ble_start; -1 timeout, -2 wrong job, -3 no tick),
 * ready = the mailbox says READY */
static void ble_guard_started(ble_guard_t *g, int rc, int ready)
{
    if (rc == 0 && ready)
        return;                                  /* trying stays until ble_guard_ok */
    g->trying = 0;
    g->blocked = 1;
    g->why = rc == -1 ? BLE_WHY_TIMEOUT : rc == -3 ? BLE_WHY_NO_TICK : rc ? BLE_WHY_WRONG_JOB : BLE_WHY_NOT_READY;
    g->fails++;
}
/* the controller ran long enough: the start is good */
static void ble_guard_ok(ble_guard_t *g)
{
    g->trying = 0;
    g->fails = 0;
}
/* BLUETOOTH set OFF in GLO: the next power-on tries again */
static void ble_guard_user_off(ble_guard_t *g)
{
    g->blocked = 0;
}
