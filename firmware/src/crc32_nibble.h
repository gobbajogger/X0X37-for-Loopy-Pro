/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared zlib CRC-32 nibble table (4 bits per step) and one update/body.
 * storage.c, project.c, slots.c, and boot_screen.h each used an identical
 * nibble table and update loop; they land in the unity TU (x0x.c), so one
 * include keeps a single XIP copy. Host tests that pull any file alone still
 * get the table and helpers. */
#ifndef X0X_CRC32_NIBBLE_H
#define X0X_CRC32_NIBBLE_H
#include <stdint.h>

#ifndef X0X_CRC32_NIBBLE_T_DEFINED
#define X0X_CRC32_NIBBLE_T_DEFINED
static const uint32_t X0X_CRC32_NIBBLE_T[16] = {
    0x00000000u, 0x1DB71064u, 0x3B6E20C8u, 0x26D930ACu, 0x76DC4190u, 0x6B6B51F4u, 0x4DB26158u, 0x5005713Cu,
    0xEDB88320u, 0xF00F9344u, 0xD6D6A3E8u, 0xCB61B38Cu, 0x9B64C2B0u, 0x86D3D2D4u, 0xA00AE278u, 0xBDBDF21Cu};
#endif

#ifndef X0X_CRC32_BODY_DEFINED
#define X0X_CRC32_BODY_DEFINED
/* Running update (no init / final invert). boot_crc_upd and st_body_ok use this.
 * noinline: keep one body in the unity TU so storage/project/slots/boot share it. */
static uint32_t x0x_crc32_upd(uint32_t c, const uint8_t *p, uint32_t n)
    __attribute__((noinline));
static uint32_t x0x_crc32_upd(uint32_t c, const uint8_t *p, uint32_t n)
{
    while (n--) {
        c ^= *p++;
        c = (c >> 4) ^ X0X_CRC32_NIBBLE_T[c & 15u];
        c = (c >> 4) ^ X0X_CRC32_NIBBLE_T[c & 15u];
    }
    return c;
}
/* Full zlib CRC-32 of n bytes at p. */
static uint32_t x0x_crc32(const void *p, uint32_t n) __attribute__((noinline));
static uint32_t x0x_crc32(const void *p, uint32_t n)
{
    return ~x0x_crc32_upd(0xFFFFFFFFu, (const uint8_t *)p, n);
}
#endif
#endif
