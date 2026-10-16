/* SPDX-License-Identifier: GPL-3.0-only */
/* SMK-37: the chip side of the Bluetooth controller image (src/ble.c does the rest).
 *
 * The controller is a separate image in the flash block at 0x8B000 (XIP 0x02087000), built by the
 * user from JieLi's files (ble/ctl). The app never links it: it checks the image header and CRC,
 * gives it boot parameters and runs its two entries on core 1 (fm1_cpu1.h). Everything else goes
 * through the HCI mailbox (x0x_ble_abi.h). This file holds the addresses and the chip values that
 * src/ may not write as numbers (tools/build.py mmio_check). */
#pragma once
#include <stdint.h>
#define XB_SYNC() __asm__ volatile("csync" ::: "memory")
#include "x0x_ble_abi.h"

#define FM1_BLE_HDR ((const xb_header_t *)XB_FLASH_XIP)
#define FM1_BLE_IMG ((const uint8_t *)XB_FLASH_XIP)
#define FM1_BLE_INIT_MS 4000u                   /* the longest the controller's start may take (its RF
                                                 * trim; the watchdog is 8 s, fed just before) */

/* WLA_CON12: the 24 MHz crystal's trim, left bits 13-16, right bits 19-22. JieLi's RF init
 * (wf_rf_common_init) writes both fields from xb_boot_t xosc[]. */
#define FM1_BLE_XOSC_REG (*(volatile uint32_t *)0x11930u)
static inline uint32_t fm1_ble_xosc(void) { return FM1_BLE_XOSC_REG; }
static inline void fm1_ble_sync(void) { XB_SYNC(); }
/* the image's init entry, called on core 0 (it moves to the image's run stack itself) */
static inline int fm1_ble_init_here(uint32_t init, const xb_boot_t *b)
{
    return ((int (*)(const xb_boot_t *))(uintptr_t)init)(b);
}   /* the two cores share the caches: order only */

/* The boot parameters. Clocks as measured on an FM-1 (docs/plans/2026-10-04-dual-core-and-clock.md:
 * system 360 MHz, HSB 180, LSB 60). RF calibration: the AC79 SDK's demo_ble board values (the unit's
 * own are not known: Stage 3 checks the link). */
static void fm1_ble_boot_fill(xb_boot_t *b, xb_mbox_t *mb)
{
    static const uint8_t pa[7] = {1, 7, 4, 7, 11, 1, 7};
    uint32_t i;
    uint8_t *z = (uint8_t *)b;
    for (i = 0; i < sizeof *b; i++)
        z[i] = 0;
    b->magic = XB_BOOT_MAGIC;
    b->abi = XB_ABI;
    b->sys_clk_hz = 360000000u;
    b->hsb_clk_hz = 180000000u;
    b->lsb_clk_hz = 60000000u;
    b->tick_addr = 0x10804u;                    /* TIMER4: free-running, 24 MHz (fm1_time.h) */
    b->ticks_per_ms = 24000u;
    b->mbox = (uint32_t)(uintptr_t)mb;
    b->settle_ms = 800u;
    b->vec_base = 0x01C7FE00u;                  /* FM1_VEC: one table for both cores */
    b->icfg_base = 0x1EEF300u;                  /* core 1's interrupt bank */
    /* a67: a fixed crystal trim 11/11, as the working SMK-37 Oscuro firmware (and the SDK default).
     * Before a67 the trim came from WLA_CON12 at boot (3/3 on the user's unit). DIAG still reads WLA_CON12. */
    b->xosc[0] = 0x0B;
    b->xosc[1] = 0x0B;
    for (i = 0; i < 7u; i++)
        b->pa_trim[i] = pa[i];
    b->osc_type = 0;
    b->pwr_max = 8;
    b->pwr_ble = 6;                             /* BLE TX level (0-11). a66: 6, as the working SMK-37 Oscuro firmware (was 11) */
}
