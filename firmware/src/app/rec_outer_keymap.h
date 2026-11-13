/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef X0X_REC_OUTER_KEYMAP_H
#define X0X_REC_OUTER_KEYMAP_H
#include <stdint.h>

/* Pack physical 0..4 and 32..36 into a ten-bit REC-only edge mask.
 * The 27 FM-1 keys 5..31 must never appear in this mask. */
static inline uint16_t rec_outer_bits(uint64_t physical)
{
    return (uint16_t)((physical & UINT64_C(0x1f)) |
                      ((physical >> 27) & UINT64_C(0x3e0)));
}

/* Packed 0..9 -> physical 0..4,32..36. */
static inline unsigned rec_outer_physical(unsigned packed)
{
    return packed < 5u ? packed : packed + 27u;
}
#endif
