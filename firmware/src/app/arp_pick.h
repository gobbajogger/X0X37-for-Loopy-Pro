/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared arpeggiator note picker (clip keybed + Chicago part arp).
 *
 * Clip mode (smk_clip_keys.h / main_fm1.c) owns the full order set, rates,
 * rhythm and MIDI→Loopy sink. Chicago live freeplay (chicago_arp.h) adapts
 * its Mode/Range/Speed enums onto this pick + CK_RATE_T subset. Keep one
 * pick body here — do not fork a second engine. */
#pragma once
#include <stdint.h>

#ifndef CK_FN
#define CK_FN __attribute__((minsize, noinline, unused)) /* small code: app area near full */
#endif

enum { CK_ORD_UP, CK_ORD_DOWN, CK_ORD_UPDN, CK_ORD_DNUP, CK_ORD_PLAYED, CK_ORD_RANDOM };
#define CK_NORD 6u
#define CK_NRATE 13u

/* Arp rates in MIDI clock ticks (24 ppqn quarter). */
static const uint8_t CK_RATE_T[CK_NRATE] = {48, 36, 32, 24, 18, 16, 12, 9, 8, 6, 4, 3, 2};
static const char CK_RATE_N[] = "1/2\0" "1/4D\0" "1/2T\0" "1/4\0" "1/8D\0" "1/4T\0" "1/8\0" "1/16D\0" "1/8T\0" "1/16\0"
                                "1/16T\0" "1/32\0" "1/32T";
static const char CK_ORD_N[] = "UP\0DOWN\0UP/DN\0DN/UP\0PLAYED\0RANDOM";

/* The arp note for a step. s: the cycle notes (sorted low to high, or in press order for PLAYED), n >= 1.
 * oct: 1..4. rnd: a random number (RANDOM only). Return -1 for a note above 127. */
CK_FN static int ck_arp_pick(const uint8_t *s, uint32_t n, uint32_t order, uint32_t oct, uint32_t step,
                             uint32_t rnd)
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
