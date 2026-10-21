/* SPDX-License-Identifier: GPL-3.0-only */
/* TEST profile only. App-owned immutable UI data, independent of BLE admission.
 * Caller excludes all flash writers from validation through the last UI read.
 * CRC detects corruption/mismatched images, not malicious replacement. */
#pragma once
#include <stdint.h>
#ifndef X0X_SMC_VENDOR_TEST
#define X0X_SMC_VENDOR_TEST 0
#endif
#define XAU_MAGIC 0x55493058u
#define XAU_VERSION 1u
#define XAU_OFF 0xAB000u
#define XAU_SIZE 8192u
typedef struct { uint32_t magic, version, off, size, crc; } xau_descriptor_t;
static inline int xau_validate(const volatile xau_descriptor_t *d,
                               const uint8_t *bytes, uint32_t available)
{
    uint32_t crc = ~0u, i, b;
    if (!d || d->magic != XAU_MAGIC || d->version != XAU_VERSION ||
        d->off != XAU_OFF || d->size != XAU_SIZE || !bytes || available != XAU_SIZE)
        return -1;
    for (i = 0; i < XAU_SIZE; ++i) {
        crc ^= bytes[i];
        for (b = 0; b < 8u; ++b)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc == d->crc ? 0 : -1;
}
/* Exclude all candidate external immutable sectors, including vendor read-only
 * data, even when BLE is disabled or its optional manifest is invalid. */
#define XAU_OWNER_END 0xAB000u
