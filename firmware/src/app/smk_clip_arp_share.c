/* SPDX-License-Identifier: GPL-3.0-only */
/* One copy of clip-mode arp pick + rate table.
 * engine.c (-O2) and x0x.c (main_fm1 / -Os) both used smk_clip_keys.h statics;
 * Chicago Arp and clip MIDI share the same helpers — keep a single definition. */
#include "smk_clip_keys.h"

const uint8_t CK_RATE_T[CK_NRATE] = {48, 36, 32, 24, 18, 16, 12, 9, 8, 6, 4, 3, 2};

#ifndef X0X_MINSIZE
#if defined(__clang__)
#define X0X_MINSIZE minsize,   /* clang (the firmware toolchain): small code */
#else
#define X0X_MINSIZE            /* gcc host tests: gcc has no minsize */
#endif
#endif
__attribute__((X0X_MINSIZE noinline)) int ck_arp_pick(const uint8_t *s, uint32_t n, uint32_t order,
                                                    uint32_t oct, uint32_t step, uint32_t rnd)
{
    uint32_t len = n * oct, i = step % len, j;
    if (order == CK_ORD_UPDN || order == CK_ORD_DNUP) {
        j = len > 1u ? step % (2u * len - 2u) : 0u;
        i = j < len ? j : 2u * len - 2u - j;
    }
    if (order == CK_ORD_DOWN || order == CK_ORD_DNUP)
        i = len - 1u - i;
    if (order == CK_ORD_RANDOM)
        i = rnd % len;
    i = s[i % n] + 12u * (i / n);
    return i <= 127u ? (int)i : -1;
}
