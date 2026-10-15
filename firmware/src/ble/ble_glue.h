/* SPDX-License-Identifier: GPL-3.0-only */
/* The parts of the Bluetooth MIDI glue (src/ble.c) that touch no hardware, so the host tests
 * (tests/ble_test.c) compile them as they are:
 *   ble_img_check()    is the flash block a controller image this app can start?
 *   ble_usb_of_msg()   a BLE-MIDI message as a USB-MIDI event packet (the queue usb.c reads)
 *   ble_msg_of_usb()   a USB-MIDI event packet as the bytes of one MIDI message
 *   ble_addr_of()      the random static address and the name from the factory record */
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
    static const char hx[] = "0123456789ABCDEF";
    uint32_t a = xb_crc32(0, rec, n), b = xb_crc32(0x5A5A5A5Au, rec, n);
    addr[0] = (uint8_t)a;
    addr[1] = (uint8_t)(a >> 8);
    addr[2] = (uint8_t)(a >> 16);
    addr[3] = (uint8_t)(a >> 24);
    addr[4] = (uint8_t)b;
    addr[5] = (uint8_t)((b >> 8) | 0xC0u);
    name[0] = 'X'; name[1] = '0'; name[2] = 'X'; name[3] = '-';
    name[4] = hx[addr[1] >> 4]; name[5] = hx[addr[1] & 15u];
    name[6] = hx[addr[0] >> 4]; name[7] = hx[addr[0] & 15u];
    name[8] = 0;
}
