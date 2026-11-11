/* SPDX-License-Identifier: GPL-3.0-only */
/* Host stand-in for fm1_usb.h. Counts reset and attach. The SIE stays off so
 * a completed write cannot clear usb.timeouts. */
#pragma once
#include <stdint.h>
#define FM1_INLINE static inline
#ifndef BOARD_SMK37
#define BOARD_SMK37 0
#endif
static unsigned fake_resets, fake_attaches;
FM1_INLINE void fm1_usb_reset(void) { fake_resets++; }
FM1_INLINE void fm1_usb_attach(void *ep0) { (void)ep0; fake_attaches++; }
FM1_INLINE void fm1_usb_off(void) {}
/* SIE off: a completed write in sie_wr would zero usb.timeouts and hide a
 * retry that forgot to clear a dead-SIE count. Attach then uses that count. */
FM1_INLINE uint32_t fm1_usb_sie_on(void) { return 0u; }
FM1_INLINE void fm1_usb_sie_wr_start(uint32_t r, uint32_t v) { (void)r; (void)v; }
FM1_INLINE void fm1_usb_sie_rd_start(uint32_t r) { (void)r; }
FM1_INLINE uint32_t fm1_usb_sie_done(void) { return 1u; }
FM1_INLINE uint32_t fm1_usb_sie_data(void) { return 0; }
FM1_INLINE void fm1_usb_ep0_buf(void *p) { (void)p; }
FM1_INLINE void fm1_usb_ep4_txbuf(void *p) { (void)p; }
FM1_INLINE void fm1_usb_ep4_send(void *p, uint32_t n) { (void)p; (void)n; }
FM1_INLINE void fm1_usb_ep_txbuf(uint32_t ep, void *p) { (void)ep; (void)p; }
FM1_INLINE void fm1_usb_ep_rxbuf(uint32_t ep, void *p) { (void)ep; (void)p; }
FM1_INLINE void fm1_usb_ep0_send(void *p, uint32_t n) { (void)p; (void)n; }
FM1_INLINE void fm1_usb_ep_send(uint32_t ep, void *p, uint32_t n) { (void)ep; (void)p; (void)n; }
FM1_INLINE void fm1_usb_rx_sync(void) {}
FM1_INLINE void fm1_usb_ep_enable(uint32_t eps) { (void)eps; }
FM1_INLINE uint32_t fm1_usb_sof_take(void) { return 0; }
