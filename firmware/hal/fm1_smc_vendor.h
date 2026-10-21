/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "x0x_ble_segment.h"
#define FM1_SMC_HDR ((const xb4_manifest_t *)(const void *)fm1_xip_ptr(XB4_PRIMARY_OFF))
static const uint8_t *fm1_smc_read(void *ctx, uint32_t off, uint32_t size)
{
    unsigned i; (void)ctx;
    for (i = 0; i < XB4_SEGMENTS; ++i)
        if (xb4_range_ok(i, off, size)) return fm1_xip_ptr(off);
    return 0;
}
static uint8_t fm1_smc_image_check(void)
{
    const xb4_manifest_t *h = FM1_SMC_HDR;
    uint32_t model = xb4_crc32((const uint8_t *)SMK_MODEL, sizeof(SMK_MODEL) - 1u);
    if (h->magic != XB4_MAGIC) return BLE_IMG_NONE;
    return xb4_validate(h, sizeof *h, model, fm1_smc_read, 0) == XB4_OK ? BLE_IMG_OK : BLE_IMG_BAD;
}
