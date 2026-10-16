/* SPDX-License-Identifier: GPL-3.0-only
 * Host test of the clip mode keybed logic (firmware/src/app/smk_clip_keys.h). */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define CK_FN __attribute__((unused))
#include "app/smk_clip_keys.h"

static uint32_t notes(uint32_t note, uint32_t m, uint32_t snap, int32_t type, uint32_t inv, uint8_t *o)
{
    memset(o, 0xEE, CK_MAXN);
    return ck_notes(note, m, snap, type, inv, o);
}

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
    assert(ck_mask(0, 0) == 0xAB5u);
    assert(ck_mask(0, 2) == 0xAD6u);                    /* D major: D E F# G A B C# */
    assert(ck_in(ck_mask(0, 2), 66) && !ck_in(ck_mask(0, 2), 65));
}

static void test_snap(void)
{
    uint8_t o[CK_MAXN];
    uint32_t c = ck_mask(0, 0);
    assert(notes(61, c, 1, -1, 0, o) == 1 && o[0] == 60);   /* C# in C major: a tie goes down */
    assert(notes(66, c, 1, -1, 0, o) == 1 && o[0] == 65);   /* F# -> F */
    assert(notes(64, c, 1, -1, 0, o) == 1 && o[0] == 64);
    assert(notes(61, c, 0, -1, 0, o) == 1 && o[0] == 61);   /* SCALE off: no snap */
    assert(notes(61, c, 2, -1, 0, o) == 0);                 /* FILTER: no note */
    assert(notes(60, c, 2, -1, 0, o) == 1 && o[0] == 60);
    assert(notes(70, ck_mask(10, 9), 1, -1, 0, o) == 1 && o[0] == 69);   /* A minor pent: A# -> A */
}

static void test_chords(void)
{
    uint8_t o[CK_MAXN];
    uint32_t c = ck_mask(0, 0);
    assert(notes(60, c, 0, 0, 0, o) == 3 && o[0] == 60 && o[1] == 64 && o[2] == 67);   /* MAJ */
    assert(notes(60, c, 0, 6, 0, o) == 4 && o[3] == 71);                                /* MAJ7 */
    assert(notes(60, c, 0, 12, 0, o) == 5 && o[4] == 74);                               /* MIN9 */
    assert(notes(62, c, 0, 14, 0, o) == 3 && o[1] == 65 && o[2] == 69);   /* SC TRIAD on D in C: D minor */
    assert(notes(67, c, 0, 15, 0, o) == 4 && o[3] == 77);                 /* SC 7TH on G in C: G7 */
    assert(notes(60, c, 0, 16, 0, o) == 5 && o[4] == 74);                 /* SC 9TH on C: D on top */
    assert(notes(60, c, 0, 0, 1, o) == 3 && o[0] == 72 && o[1] == 64);    /* 1st inversion */
    assert(notes(60, c, 0, 0, 3, o) == 3 && o[2] == 67 && o[1] == 76);    /* INV 3 on a triad: 2 notes up */
    assert(notes(61, c, 1, 0, 0, o) == 3 && o[0] == 60);                  /* snap, then the chord */
    assert(notes(84, c, 0, 13, 3, o) == 5);                               /* the highest key: in range */
    for (int i = 0; i < 5; i++)
        assert(o[i] <= 127);
}

static void test_arp(void)
{
    static const uint8_t n[3] = {60, 64, 67};
    int i, up[6];
    for (i = 0; i < 6; i++)
        up[i] = ck_arp_pick(n, 3, CK_ORD_UP, 2, (uint32_t)i, 0);
    assert(up[0] == 60 && up[2] == 67 && up[3] == 72 && up[5] == 79);   /* 2 octaves */
    assert(ck_arp_pick(n, 3, CK_ORD_DOWN, 1, 0, 0) == 67);
    {   /* UP/DN on 3 notes: 60 64 67 64 60 64; DN/UP: 67 64 60 64 67 64 */
        static const int ud[6] = {60, 64, 67, 64, 60, 64}, du[6] = {67, 64, 60, 64, 67, 64};
        for (i = 0; i < 6; i++) {
            assert(ck_arp_pick(n, 3, CK_ORD_UPDN, 1, (uint32_t)i, 0) == ud[i]);
            assert(ck_arp_pick(n, 3, CK_ORD_DNUP, 1, (uint32_t)i, 0) == du[i]);
        }
    }
    assert(ck_arp_pick(n, 3, CK_ORD_PLAYED, 1, 1, 0) == 64);
    for (i = 0; i < 50; i++) {
        int v = ck_arp_pick(n, 3, CK_ORD_RANDOM, 3, 0, (uint32_t)i * 2654435761u);
        assert(v >= 60 && v <= 91);
    }
    assert(ck_arp_pick(n, 1, CK_ORD_UPDN, 1, 5, 0) == 60);
    {
        static const uint8_t hi[1] = {120};
        assert(ck_arp_pick(hi, 1, CK_ORD_UP, 2, 1, 0) == -1);            /* above 127: no note */
    }
}

static void test_tables(void)
{
    assert(CK_RATE_T[3] == 24 && CK_RATE_T[9] == 6 && CK_RATE_T[12] == 2);   /* 1/4, 1/16, 1/32T */
    assert(!strcmp(ck_nm(CK_RATE_N, 9), "1/16") && !strcmp(ck_nm(CK_RATE_N, 12), "1/32T"));
    assert(!strcmp(ck_nm(CK_SCALE_N, 10), "MIN PENT") && !strcmp(ck_nm(CK_CHORD_N, 16), "SC 9TH"));
    assert(!strcmp(ck_nm(CK_ROOT_N, 11), "B") && !strcmp(ck_nm(CK_ORD_N, 5), "RANDOM"));
}

int main(void)
{
    test_scale_masks();
    test_snap();
    test_chords();
    test_arp();
    test_tables();
    printf("smk_clip_keys_test: ok\n");
    return 0;
}
