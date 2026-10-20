/* SPDX-License-Identifier: GPL-3.0-only */
/* A host model of firmware/hal/fm1_usb.h for tests/host/smk_clip_usb_clear_test.c. The test script
 * copies usb.c next to it, so usb.c "../hal/fm1_usb.h" opens this file. It models the SIE byte
 * registers that usb_poll reads: the interrupt flags (cleared on read), the frame number, and the
 * EP1 OUT status. The test sets them (fake_frame, fake_sof, fake_intrusb, fake_rx). */
#pragma once
#include <stdint.h>
#define FM1_INLINE static inline
static uint8_t fake_reg[8][32];         /* [INDEX][register]; registers below 16 use INDEX 0 */
static uint32_t fake_rd_r, fake_index;
static uint16_t fake_frame;             /* SOF frame number: it moves while a host is there */
static uint8_t fake_sof, fake_intrusb, fake_rx;
static uint8_t *fake_slot(uint32_t r) { return &fake_reg[r >= 16u ? fake_index & 7u : 0u][r & 31u]; }
FM1_INLINE void fm1_usb_reset(void) {}
FM1_INLINE void fm1_usb_attach(void *ep0) { (void)ep0; }
FM1_INLINE void fm1_usb_off(void) {}
FM1_INLINE uint32_t fm1_usb_sie_on(void) { return 1u; }
FM1_INLINE void fm1_usb_sie_wr_start(uint32_t r, uint32_t v)
{
    if (r == 14u)
        fake_index = v;
    if (r == 20u && fake_index == 1u)
        v &= ~1u;                       /* RXCSR1: the driver took the packet */
    *fake_slot(r) = (uint8_t)v;
}
FM1_INLINE void fm1_usb_sie_rd_start(uint32_t r) { fake_rd_r = r; }
FM1_INLINE uint32_t fm1_usb_sie_done(void) { return 1u; }
FM1_INLINE uint32_t fm1_usb_sie_data(void)
{
    uint32_t r = fake_rd_r, v;
    if (r == 6u) { v = fake_intrusb; fake_intrusb = 0; return v; }       /* INTRUSB */
    if (r == 4u) { v = fake_rx ? 0x02u : 0u; return v; }                 /* INTRRX1: EP1 OUT */
    if (r == 2u) return 0u;                                              /* INTRTX1 */
    if (r == 12u) return fake_frame & 0xFFu;
    if (r == 13u) return (fake_frame >> 8) & 7u;
    if (r == 20u && fake_index == 1u) { v = fake_rx ? 1u : 0u; fake_rx = 0; return v; }   /* RxPktRdy */
    return *fake_slot(r);
}
FM1_INLINE void fm1_usb_ep0_buf(void *p) { (void)p; }
FM1_INLINE void fm1_usb_ep4_txbuf(void *p) { (void)p; }
FM1_INLINE void fm1_usb_ep4_send(void *p, uint32_t n) { (void)p; (void)n; }
FM1_INLINE void fm1_usb_ep_txbuf(uint32_t ep, void *p) { (void)ep; (void)p; }
FM1_INLINE void fm1_usb_ep_rxbuf(uint32_t ep, void *p) { (void)ep; (void)p; }
FM1_INLINE void fm1_usb_ep0_send(void *p, uint32_t n) { (void)p; (void)n; }
FM1_INLINE void fm1_usb_ep_send(uint32_t ep, void *p, uint32_t n) { (void)ep; (void)p; (void)n; }
FM1_INLINE void fm1_usb_rx_sync(void) {}
FM1_INLINE void fm1_usb_ep_enable(uint32_t eps) { (void)eps; }
FM1_INLINE uint32_t fm1_usb_sof_take(void) { uint32_t v = fake_sof; fake_sof = 0; return v; }
