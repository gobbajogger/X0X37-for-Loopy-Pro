/* SPDX-License-Identifier: GPL-3.0-only */
/* SIE byte-register model for real usb.c. Only EP4 OUT is driven by this test;
 * unused controller operations are inert, with no physical device access. */
#pragma once
#include <stdint.h>
static uint8_t fake_reg[8][32];
static uint32_t fake_index, fake_rd_r;
static uint8_t *fake_slot(uint32_t r)
{
    return &fake_reg[r >= 16u ? fake_index & 7u : 0u][r & 31u];
}
static inline void fm1_usb_reset(void) {}
static inline void fm1_usb_attach(void *p) { (void)p; }
static inline void fm1_usb_off(void) {}
static inline uint32_t fm1_usb_sie_on(void) { return 1u; }
static inline void fm1_usb_sie_wr_start(uint32_t r, uint32_t v)
{
    if (r == 14u)
        fake_index = v;
    if (r == 20u && fake_index == 4u)
        v &= ~1u;                 /* RXCSR1: acknowledge/re-arm the OUT packet */
    *fake_slot(r) = (uint8_t)v;
}
static inline void fm1_usb_sie_rd_start(uint32_t r) { fake_rd_r = r; }
static inline uint32_t fm1_usb_sie_done(void) { return 1u; }
static inline uint32_t fm1_usb_sie_data(void) { return *fake_slot(fake_rd_r); }
static inline void fm1_usb_ep0_buf(void *p) { (void)p; }
static inline void fm1_usb_ep4_txbuf(void *p) { (void)p; }
static inline void fm1_usb_ep4_rxbuf(void *p) { (void)p; }
static inline void fm1_usb_ep4_send(void *p, uint32_t n) { (void)p; (void)n; }
static inline void fm1_usb_ep_txbuf(uint32_t ep, void *p) { (void)ep; (void)p; }
static inline void fm1_usb_ep_rxbuf(uint32_t ep, void *p) { (void)ep; (void)p; }
static inline void fm1_usb_ep0_send(void *p, uint32_t n) { (void)p; (void)n; }
static inline void fm1_usb_ep_send(uint32_t ep, void *p, uint32_t n) { (void)ep; (void)p; (void)n; }
static inline void fm1_usb_rx_sync(void) {}
static inline void fm1_usb_ep_enable(uint32_t m) { (void)m; }
static inline uint32_t fm1_usb_sof_take(void) { return 0u; }
