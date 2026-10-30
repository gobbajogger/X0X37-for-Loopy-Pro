/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_ZERO_H
#define ORIGINAL_MIAMI_ZERO_H
#include <stdint.h>

/* Batch exact positive-zero stores. No byte-at-a-time libc loop is needed
 * for settled idle buffers. Signed scratch/dry arrays use the corresponding
 * unsigned type, which may alias them; float outputs retain their own type. */
#define MIAMI_ZERO_2(p) do { (p)[0] = 0; (p)[1] = 0; } while (0)
#define MIAMI_ZERO_4(p) do { MIAMI_ZERO_2(p); MIAMI_ZERO_2((p) + 2); } while (0)
#define MIAMI_ZERO_8(p) do { MIAMI_ZERO_4(p); MIAMI_ZERO_4((p) + 4); } while (0)
#define MIAMI_ZERO_16(p) do { MIAMI_ZERO_8(p); MIAMI_ZERO_8((p) + 8); } while (0)
#define MIAMI_ZERO_32(p) do { MIAMI_ZERO_16(p); MIAMI_ZERO_16((p) + 16); } while (0)

static inline void original_miami_zero_words(uint32_t *p, uint32_t n)
{
    while (n >= 32) { MIAMI_ZERO_32(p); p += 32; n -= 32; }
    if (n & 16) { MIAMI_ZERO_16(p); p += 16; }
    if (n & 8) { MIAMI_ZERO_8(p); p += 8; }
    if (n & 4) { MIAMI_ZERO_4(p); p += 4; }
    if (n & 2) { MIAMI_ZERO_2(p); p += 2; }
    if (n & 1) *p = 0;
}

static inline void original_miami_zero_halves(uint16_t *p, uint32_t n)
{
    while (n >= 32) { MIAMI_ZERO_32(p); p += 32; n -= 32; }
    if (n & 16) { MIAMI_ZERO_16(p); p += 16; }
    if (n & 8) { MIAMI_ZERO_8(p); p += 8; }
    if (n & 4) { MIAMI_ZERO_4(p); p += 4; }
    if (n & 2) { MIAMI_ZERO_2(p); p += 2; }
    if (n & 1) *p = 0;
}

static inline void original_miami_zero_floats(float *p, uint32_t n)
{
    while (n >= 32) { MIAMI_ZERO_32(p); p += 32; n -= 32; }
    if (n & 16) { MIAMI_ZERO_16(p); p += 16; }
    if (n & 8) { MIAMI_ZERO_8(p); p += 8; }
    if (n & 4) { MIAMI_ZERO_4(p); p += 4; }
    if (n & 2) { MIAMI_ZERO_2(p); p += 2; }
    if (n & 1) *p = 0;
}
#undef MIAMI_ZERO_2
#undef MIAMI_ZERO_4
#undef MIAMI_ZERO_8
#undef MIAMI_ZERO_16
#undef MIAMI_ZERO_32

#endif
