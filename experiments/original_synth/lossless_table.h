/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_LOSSLESS_TABLE_H
#define ORIGINAL_LOSSLESS_TABLE_H
#include <stdint.h>
#include <stddef.h>
/* 32-entry blocks, exact raw binary32 bits. Per block: first LE32, width byte,
 * then LSB-first fixed-width zigzag modulo32 deltas. No approximation/libm. */
typedef struct {const uint8_t *data;const uint32_t *offset;size_t bytes;uint32_t count,blocks;} original_lossless_table;
int original_lossless_lookup(const original_lossless_table *table,uint32_t index,uint32_t *bits);
/* Separate second-order format: first LE32, first-delta LE32, width byte,
 * then zigzag second differences. Fixed32 blocks, same descriptor, no old
 * format autodetection or reinterpretation. Empty final residual stream valid. */
int original_lossless_second_lookup(const original_lossless_table *table,uint32_t index,uint32_t *bits);
/* Same second-order payload with a separate 16-bit byte-offset directory.
 * This descriptor is valid only for payloads of at most UINT16_MAX bytes.
 * The original descriptor/API remains the fallback for larger payloads. */
typedef struct {const uint8_t *data;const uint16_t *offset;size_t bytes;uint32_t count,blocks;} original_lossless_table16;
int original_lossless_second16_lookup(const original_lossless_table16 *table,uint32_t index,uint32_t *bits);
#endif
