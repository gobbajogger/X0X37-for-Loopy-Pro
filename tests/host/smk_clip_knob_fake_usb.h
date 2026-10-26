/* SPDX-License-Identifier: GPL-3.0-only */
/* A host model of firmware/hal/fm1_usb.h for tests/host/smk_clip_knob_test.c. As
 * smk_clip_usb_clear_fake_usb.h, and it also keeps what usb.c sends on EP1 IN (fake_tx), and the host
 * takes each IN packet at once (TXCSR1 TxPktRdy clears). */
#pragma once
#include <stdint.h>
#define FM1_INLINE static inline
static uint8_t fake_reg[8][32];
static uint32_t fake_rd_r, fake_index;
static uint16_t fake_frame;
static uint8_t fake_sof, fake_intrusb, fake_rx;
static uint32_t fake_tx[256];           /* the USB-MIDI event packets that went to the host */
static unsigned fake_tx_n;
static uint8_t *fake_slot(uint32_t r) { return &fake_reg[r >= 16u ? fake_index & 7u : 0u][r & 31u]; }
FM1_INLINE void fm1_usb_reset(void) {}
FM1_INLINE void fm1_usb_attach(void *ep0) { (void)ep0; }
FM1_INLINE void fm1_usb_off(void) {}
FM1_INLINE uint32_t fm1_usb_sie_on(void) { return 1u; }
FM1_INLINE void fm1_usb_sie_wr_start(uint32_t r, uint32_t v)
{
    if (r == 14u)
        fake_index = v;
    if ((r == 20u || r == 17u) && fake_index == 1u)
        v &= ~1u;                       /* RXCSR1: the driver took the packet; TXCSR1: the host took it */
    *fake_slot(r) = (uint8_t)v;
}
FM1_INLINE void fm1_usb_sie_rd_start(uint32_t r) { fake_rd_r = r; }
FM1_INLINE uint32_t fm1_usb_sie_done(void) { return 1u; }
FM1_INLINE uint32_t fm1_usb_sie_data(void)
{
    uint32_t r = fake_rd_r, v;
    if (r == 6u) { v = fake_intrusb; fake_intrusb = 0; return v; }
    if (r == 4u) { v = fake_rx ? 0x02u : 0u; return v; }
    if (r == 2u) return 0u;
    if (r == 12u) return fake_frame & 0xFFu;
    if (r == 13u) return (fake_frame >> 8) & 7u;
    if (r == 20u && fake_index == 1u) { v = fake_rx ? 1u : 0u; fake_rx = 0; return v; }
    return *fake_slot(r);
}
FM1_INLINE void fm1_usb_ep0_buf(void *p) { (void)p; }
FM1_INLINE void fm1_usb_ep4_txbuf(void *p) { (void)p; }
FM1_INLINE void fm1_usb_ep4_send(void *p, uint32_t n) { (void)p; (void)n; }
FM1_INLINE void fm1_usb_ep_txbuf(uint32_t ep, void *p) { (void)ep; (void)p; }
FM1_INLINE void fm1_usb_ep_rxbuf(uint32_t ep, void *p) { (void)ep; (void)p; }
FM1_INLINE void fm1_usb_ep0_send(void *p, uint32_t n) { (void)p; (void)n; }
FM1_INLINE void fm1_usb_ep_send(uint32_t ep, void *p, uint32_t n)
{
    const uint8_t *b = (const uint8_t *)p;
    uint32_t i;
    if (ep != 1u)
        return;
    for (i = 0; i + 4u <= n && fake_tx_n < 256u; i += 4u)
        fake_tx[fake_tx_n++] = b[i] | (uint32_t)b[i + 1] << 8 | (uint32_t)b[i + 2] << 16 | (uint32_t)b[i + 3] << 24;
}
FM1_INLINE void fm1_usb_rx_sync(void) {}
FM1_INLINE void fm1_usb_ep_enable(uint32_t eps) { (void)eps; }
FM1_INLINE uint32_t fm1_usb_sof_take(void) { uint32_t v = fake_sof; fake_sof = 0; return v; }
