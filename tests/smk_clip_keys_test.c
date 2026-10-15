/* SPDX-License-Identifier: GPL-3.0-only
 * Host test of the clip mode keybed logic (firmware/src/app/smk_clip_keys.h). */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define CK_FN __attribute__((unused))
#include "app/smk_clip_keys.h"

static void test_scale_masks(void)
{
    static const uint8_t MAJ[7] = {0, 2, 4, 5, 7, 9, 11}, HMIN[7] = {0, 2, 3, 5, 7, 8, 11};
    int i;
    for (i = 0; i < 7; i++) {
        assert((CK_SCALE_MASK[0] >> MAJ[i]) & 1u);
        assert((CK_SCALE_MASK[7] >> HMIN[i]) & 1u);
    }
    for (i = 0; i < (int)CK_NSCALE; i++)
        assert(CK_SCALE_MASK[i] & 1u);                 /* every scale has its root */
    assert(__builtin_popcount(CK_SCALE_MASK[9]) == 5 && __builtin_popcount(CK_SCALE_MASK[10]) == 5);
}

static void test_snap(void)
{
    assert(ck_snap(61, 0, CK_SCALE_MASK[0]) == 60);     /* C# in C major: a tie goes down */
    assert(ck_snap(66, 0, CK_SCALE_MASK[0]) == 65);     /* F# -> F */
    assert(ck_snap(64, 0, CK_SCALE_MASK[0]) == 64);
    assert(ck_snap(63, 9, CK_SCALE_MASK[1]) == 62 || ck_snap(63, 9, CK_SCALE_MASK[1]) == 64);
    assert(ck_snap(1, 0, CK_SCALE_MASK[0]) == 0);
    assert(ck_snap(127, 0, CK_SCALE_MASK[0]) <= 127);
}

static void test_chords(void)
{
    uint8_t o[CK_MAXN];
    assert(ck_chord(60, 0, o) == 3 && o[0] == 60 && o[1] == 64 && o[2] == 67);
    assert(ck_chord(60, 6, o) == 4 && o[3] == 71);       /* MAJ7 */
    assert(ck_chord(60, 12, o) == 5 && o[4] == 74);      /* MIN9 */
    assert(ck_chord(125, 0, o) == 1);                    /* above 127: dropped */
}

static void test_arp(void)
{
    static const uint8_t n[3] = {67, 60, 64};
    int up[6], i;
    for (i = 0; i < 6; i++)
        up[i] = ck_arp_pick(n, 3, CK_ORD_UP, 2, (uint32_t)i);
    assert(up[0] == 60 && up[1] == 64 && up[2] == 67 && up[3] == 72 && up[5] == 79);
    assert(ck_arp_pick(n, 3, CK_ORD_DOWN, 1, 0) == 67);
    assert(ck_arp_pick(n, 3, CK_ORD_PLAYED, 1, 1) == 60);
    {   /* UP/DN on 3 notes: 60 64 67 64 60 64 ... */
        static const int want[6] = {60, 64, 67, 64, 60, 64};
        for (i = 0; i < 6; i++)
            assert(ck_arp_pick(n, 3, CK_ORD_UPDN, 1, (uint32_t)i) == want[i]);
    }
    for (i = 0; i < 50; i++) {
        int v = ck_arp_pick(n, 3, CK_ORD_RANDOM, 3, (uint32_t)i);
        assert(v >= 60 && v <= 91);
    }
    assert(ck_arp_pick(n, 0, CK_ORD_UP, 1, 0) == -1);
    assert(ck_arp_pick(n, 1, CK_ORD_UPDN, 1, 5) == 67);
}

static void test_rates_gate(void)
{
    assert(CK_RATE_T[3] == 24 && CK_RATE_T[9] == 6 && CK_RATE_T[12] == 2);   /* 1/4, 1/16, 1/32T */
    assert(!strcmp(ck_nm(CK_RATE_N, 9), "1/16") && !strcmp(ck_nm(CK_RATE_N, 12), "1/32T"));
    assert(!strcmp(ck_nm(CK_SCALE_N, 10), "MIN PENT") && !strcmp(ck_nm(CK_CHORD_N, 13), "DOM9"));
    assert(!strcmp(ck_nm(CK_ROOT_N, 11), "B") && !strcmp(ck_nm(CK_ORD_N, 0), "UP"));
    assert(ck_gate_ticks(6, 5) == 3 && ck_gate_ticks(2, 1) == 1 && ck_gate_ticks(6, 10) == 6);
}

int main(void)
{
    test_scale_masks();
    test_snap();
    test_chords();
    test_arp();
    test_rates_gate();
    puts("smk_clip_keys_test: ok");
    return 0;
}
