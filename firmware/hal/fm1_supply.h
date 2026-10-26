/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 genkigenki */
/* The stock supply setup before the Bluetooth controller starts (BLE stage 3, attempt 27).
 * Register order and values as described for the stock FM-1 V15 firmware by Melodee
 * (github.com/keremimo/melodee, BUILDING.md "The stock supply setup"; AC79 SDK __hw_power_init):
 * LDO13 on, DCDC13 on, LDO13 off; P3 analog 1/5/6/7/8; SYSVDD drive; VDDIOM 4, VDC14 7 (1.60 V),
 * VDDIOW 3; RC 250k on, reg 0x16 bits 0-5 clear, VDDOK_DIS, reg 0x17 bit 5 clear; SYSVDD 15 last.
 * Written here from that description. Call with IRQs off, core 1 not started.
 * Returns 1 when every transfer ended and the read-back matches, else 0 (the sequence stops). */
#pragma once
#include <stdint.h>
#include "fm1_sys.h"

static inline __attribute__((always_inline)) void fm1_supply_wait(void)
{
    volatile uint32_t n = 1000u;                 /* about the stock delay(1000) */
    while (n--)
        __asm__ volatile("nop");
}

static int fm1_supply_set(uint32_t reg, uint32_t lo, uint32_t bits, uint32_t v)
{
    uint32_t t = fm1_p33_timeouts, m = ((1u << bits) - 1u) << lo;
    uint8_t old = fm1_p33_read(reg);
    if (fm1_p33_timeouts != t)
        return 0;                                /* never write back a dummy byte */
    fm1_p33_write(reg, (uint8_t)((old & ~m) | ((v << lo) & m)));
    return fm1_p33_timeouts == t;
}

static uint32_t fm1_supply_stock(void)
{
    uint32_t t = fm1_p33_timeouts;
    static const uint8_t ana[5][2] = {{1, 0x20}, {5, 0x40}, {6, 0x0C}, {7, 0xA5}, {8, 0x96}};
    uint32_t i;
    uint8_t r0, r5, r6, r9;
    if (!fm1_supply_set(0x00u, 2, 1, 1)) return 0;   /* LDO13 on */
    fm1_supply_wait();
    if (!fm1_supply_set(0x00u, 3, 1, 1)) return 0;   /* DCDC13 on */
    fm1_supply_wait();
    if (!fm1_supply_set(0x00u, 2, 1, 0)) return 0;   /* LDO13 off */
    fm1_supply_wait();
    for (i = 0; i < 5u; i++)
        fm1_p33_write(ana[i][0], ana[i][1]);
    if (fm1_p33_timeouts != t) return 0;
    if (!fm1_supply_set(0x09u, 4, 1, 1)) return 0;   /* SYSVDD drive */
    if (!fm1_supply_set(0x05u, 0, 3, 4)) return 0;   /* VDDIOM 4 */
    if (!fm1_supply_set(0x06u, 0, 3, 7)) return 0;   /* VDC14 7: 1.60 V */
    if (!fm1_supply_set(0x05u, 4, 2, 3)) return 0;   /* VDDIOW 3 */
    fm1_p33_or(0x00u, 0x80u);                         /* RC 250k on */
    if (fm1_p33_timeouts != t) return 0;
    if (!fm1_supply_set(0x16u, 0, 6, 0)) return 0;
    fm1_p33_or(0x02u, 0x40u);                         /* VDDOK_DIS */
    fm1_p33_and(0x17u, 0xDFu);
    if (fm1_p33_timeouts != t) return 0;
    if (!fm1_supply_set(0x09u, 0, 4, 15)) return 0;  /* SYSVDD 15 (1.38 V), last */
    r0 = fm1_p33_read(0x00u);
    r5 = fm1_p33_read(0x05u);
    r6 = fm1_p33_read(0x06u);
    r9 = fm1_p33_read(0x09u);
    return fm1_p33_timeouts == t && (r0 & 0x0Cu) == 0x08u && (r5 & 0x37u) == 0x34u &&
           (r6 & 7u) == 7u && (r9 & 15u) == 15u;
}
