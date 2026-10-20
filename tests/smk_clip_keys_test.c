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

/* the count of names in a list (each one ends with a 0 byte; the list ends at its size) */
static uint32_t nm_count(const char *s, size_t size)
{
    uint32_t n = 0;
    size_t i;
    for (i = 0; i < size; i++)
        n += !s[i];
    return n;
}

static void test_new_scales_chords(void)
{
    /* every chord as intervals above the root (the SC types are tested above) */
    static const uint8_t ABS[CK_NCHORD][7] = {
        {0, 4, 7}, {0, 3, 7}, {0, 3, 6}, {0, 4, 8}, {0, 2, 7}, {0, 5, 7}, {0, 4, 7, 11}, {0, 3, 7, 10},
        {0, 4, 7, 10}, {0, 3, 6, 10}, {0, 3, 6, 9}, {0, 4, 7, 11, 14}, {0, 3, 7, 10, 14}, {0, 4, 7, 10, 14},
        {0}, {0}, {0}, {0, 4, 7, 9}, {0, 3, 7, 9}, {0, 3, 7, 11}, {0, 5, 7, 10}, {0, 4, 8, 10}, {0, 4, 7, 14},
        {0, 3, 7, 14}, {0, 4, 7, 11, 14, 17}, {0, 3, 7, 10, 14, 17}, {0, 4, 7, 10, 14, 17},
        {0, 4, 7, 11, 14, 17, 21}, {0, 3, 7, 10, 14, 17, 21}, {0, 4, 7, 10, 14, 17, 21}};   /* all the tones */
    static const uint8_t SIZE[CK_NCHORD] = {3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 3, 4, 5,
                                            4, 4, 4, 4, 4, 4, 4, 6, 6, 6, 7, 7, 7};
    uint8_t o[CK_MAXN];
    uint32_t t, i, c = ck_mask(0, 0);
    assert(nm_count(CK_SCALE_N, sizeof CK_SCALE_N) == CK_NSCALE);
    assert(nm_count(CK_CHORD_N, sizeof CK_CHORD_N) == CK_NCHORD);
    assert(nm_count(CK_RHY_N, sizeof CK_RHY_N) == CK_NRHY);
    assert(!strcmp(ck_nm(CK_CHORD_N, 20), "7SUS4") && !strcmp(ck_nm(CK_CHORD_N, 29), "DOM13"));
    assert(!strcmp(ck_nm(CK_SCALE_N, 11), "CHROMATIC") && !strcmp(ck_nm(CK_SCALE_N, 23), "DBL HARM"));
    assert(CK_SCALE_MASK[11] == 0xFFFu && __builtin_popcount(CK_SCALE_MASK[18]) == 6);   /* WHOLE TONE */
    assert(CK_SCALE_MASK[12] == (1u | 1u << 3 | 1u << 5 | 1u << 6 | 1u << 7 | 1u << 10));   /* BLUES */
    for (t = 0; t < CK_NSCALE; t++)
        assert(CK_SCALE_MASK[t] & 1u && CK_SCALE_MASK[t] <= 0xFFFu);
    for (t = 0; t < CK_NCHORD; t++) {
        assert(notes(60, c, 0, (int32_t)t, 0, o) == SIZE[t]);
        if (t - 14u < 3u)
            continue;
        for (i = 0; i < SIZE[t]; i++)
            assert(o[i] == 60u + ABS[t][i]);
        assert(notes(84, c, 0, (int32_t)t, 3, o) == SIZE[t]);   /* the highest key, inversion 3: in range */
        for (i = 0; i < SIZE[t]; i++)
            assert(o[i] <= 127u);
    }
}

static void test_rhythm(void)
{
    uint32_t r, s, l, h, hits, pos;
    for (s = 0; s < 40; s++)
        assert(ck_rhy_hit(0, s) && ck_rhy_pos(0, s) == s);   /* OFF: every step plays */
    for (r = 1; r < CK_NRHY; r++) {
        l = (CK_RHY_T[r] & 15u) + 1u;
        h = CK_RHY_T[r] >> 4;
        assert(h >= 1u && h < l);
        {   /* the name is hits/steps */
            char t[8];
            snprintf(t, sizeof t, "%u/%u", h, l);
            assert(!strcmp(ck_nm(CK_RHY_N, r), t));
        }
        for (s = hits = 0; s < 3u * l; s++) {
            pos = ck_rhy_pos(r, s);
            assert(pos == hits);                            /* the note number: the hits before this step */
            hits += ck_rhy_hit(r, s);
        }
        assert(hits == 3u * h);
        assert(ck_rhy_hit(r, 0));                           /* the first step is a hit */
    }
    assert(ck_rhy_hit(8, 0) && !ck_rhy_hit(8, 1) && !ck_rhy_hit(8, 2) && ck_rhy_hit(8, 3));   /* 3/8: x..x..x. */
    assert(!ck_rhy_hit(8, 4) && !ck_rhy_hit(8, 5) && ck_rhy_hit(8, 6) && !ck_rhy_hit(8, 7));
}

/* Stage buttons open the settings page on press and close it on the next press. */
static void test_page(void)
{
    uint32_t pg = 0;
    pg = ck_page_press(pg, 1u, 1u);
    assert((pg & 3u) == 1u && (pg >> 4) == CK_E_ON);
    assert(ck_page_field(1u, CK_E_ON, 1) == CK_E_ROOT);
    assert(ck_page_field(1u, CK_E_ROOT, -1) == CK_E_ON);
    assert(ck_page_field(1u, CK_E_SNAP, 1) == CK_E_ON);
    assert(ck_page_field(2u, CK_E_ON, 1) == CK_E_CHORD);
    assert(ck_page_field(3u, CK_E_LATCH, 1) == CK_E_ON);
    pg = ck_page_press(pg, 0u, 0u);               /* release leaves the page open */
    assert((pg & 3u) == 1u);
    pg = ck_page_press(pg, 2u, 2u);               /* another stage replaces it */
    assert((pg & 3u) == 2u && (pg >> 4) == CK_E_ON);
    pg = ck_page_press(pg, 2u, 2u);               /* second press closes it */
    assert(pg == 0u);
    pg = ck_page_press(pg, 4u, 8u | 4u);          /* NOTE REPEAT + ARP keeps LATCH */
    assert(pg == 0u);
    pg = ck_page_press(pg, 4u, 4u);
    assert((pg & 3u) == 3u);
}

/* < and > (bit 0 <, bit 1 >): a press steps at once, a hold repeats after CK_RPT_DELAY, then each CK_RPT_MS */
static void test_rpt(void)
{
    uint32_t t = 0;
    uint32_t now, n = 0;
    assert(ck_rpt(0, 0, 100, &t) == 0);                       /* nothing held: no step */
    assert(ck_rpt(2, 2, 65530, &t) == 1);                     /* > press: one step up at once (the ms wraps) */
    for (now = 65531; now < 65530u + CK_RPT_DELAY; now++)
        assert(ck_rpt(2, 0, now, &t) == 0);                   /* no repeat before the delay */
    assert(ck_rpt(2, 0, 65530u + CK_RPT_DELAY, &t) == 1);
    for (now = 65531u + CK_RPT_DELAY; now <= 65530u + CK_RPT_DELAY + 10u * CK_RPT_MS; now++)
        n += ck_rpt(2, 0, now, &t) == 1;
    assert(n == 10);                                          /* then one step each CK_RPT_MS */
    assert(ck_rpt(2, 0, now + 1000u, &t) == 1 && ck_rpt(2, 0, now + 1001u, &t) == 0);   /* a long pass: no burst */
    assert(ck_rpt(0, 0, 70000, &t) == 0 && ck_rpt(0, 0, 70500, &t) == 0);   /* release: no step */
    assert(ck_rpt(1, 1, 71000, &t) == -1);                    /* < press: one step down */
    assert(ck_rpt(1, 0, 71000u + CK_RPT_DELAY, &t) == -1);
    assert(ck_rpt(3, 2, 71500, &t) == 0);                     /* < and > held: no step */
    assert(ck_rpt(3, 0, 72500, &t) == 0);
    for (now = 72600; now < 74000; now++)                     /* > up: < does nothing until a new press */
        assert(ck_rpt(1, 0, now, &t) == 0);
    assert(ck_rpt(0, 0, 74000, &t) == 0 && ck_rpt(1, 1, 74100, &t) == -1);
    /* > goes down in a menu or with no page (lr 0), and stays down when the menu closes: no step, no repeat */
    assert(ck_rpt(0, 0, 80000, &t) == 0);
    for (now = 80001; now < 82000; now++)
        assert(ck_rpt(2, 0, now, &t) == 0);
    assert(ck_rpt(0, 0, 82000, &t) == 0);                     /* release, then a new press: steps */
    assert(ck_rpt(2, 2, 82100, &t) == 1);
    assert(ck_rpt(2, 0, 82100u + CK_RPT_DELAY, &t) == 1);
    assert(ck_rpt(0, 0, 83000, &t) == 0);                     /* a menu opens during a hold: stop */
    for (now = 83001; now < 84000; now++)
        assert(ck_rpt(2, 0, now, &t) == 0);                   /* it closes, > still down: nothing */
    /* the page closes (or clip mode stops: the page closes) during a hold, then shows again: nothing */
    assert(ck_rpt(1, 1, 90000, &t) == -1 && ck_rpt(1, 0, 90000u + CK_RPT_DELAY, &t) == -1);
    assert(ck_rpt(0, 0, 90500, &t) == 0);
    for (now = 90501; now < 92000; now++)
        assert(ck_rpt(1, 0, now, &t) == 0);
    /* the field at its lowest and highest value: no step, no wrap; a one-value field never moves */
    assert(ck_step(0, -1, 7) == 0 && ck_step(7, 1, 7) == 7);
    assert(ck_step(0, 1, 7) == 1 && ck_step(7, -1, 7) == 6 && ck_step(3, 1, 7) == 4 && ck_step(3, -1, 7) == 2);
    assert(ck_step(0, 1, 0) == 0 && ck_step(0, -1, 0) == 0);
    assert(ck_step(1, 1, 1) == 1 && ck_step(1, -1, 1) == 0);   /* LATCH, SNAP: 0..1 */
    assert(ck_step(23, 1, 23) == 23 && ck_step(29, 1, 29) == 29);   /* the last scale, the last chord */
}

static void test_human(void)
{
    uint32_t h, i, late, v, r, seen_max = 0;
    for (i = 0; i < 2000; i++) {                            /* HUMAN 0: no change, no wait */
        r = i * 2654435761u;
        assert(ck_human(0, r, 100, &late) == 100 && late == 0);
    }
    for (h = 1; h <= 7; h++)
        for (i = 0; i < 4000; i++) {
            r = (i ^ h << 20) * 2654435761u;
            v = ck_human(h, r, 64, &late);
            assert(late <= 4u * h && late <= CK_LATE_MAX);
            assert((int)v >= 64 - 4 * (int)h && (int)v <= 64 + 4 * (int)h);
            assert(ck_human(h, r, 1, &late) >= 1u && ck_human(h, r, 127, &late) <= 127u);
            if (h == 7 && late > seen_max)
                seen_max = late;
        }
    assert(seen_max == CK_LATE_MAX);                        /* HUMAN 7 reaches 28 ms */
    /* the queue's due test: early in the 28 ms before the time, due at the time and after it */
    for (i = 0; i < 256; i++) {
        uint32_t t = (i + 17u) & 255u;
        assert(ck_q_due(t, i + 17u));
        assert(!ck_q_due(t, i + 17u - 1u) && !ck_q_due(t, i + 17u - CK_LATE_MAX));
        assert(ck_q_due(t, i + 17u + 1u) && ck_q_due(t, i + 17u + 200u));   /* a long pass: late, not lost */
    }
}

int main(void)
{
    test_scale_masks();
    test_snap();
    test_chords();
    test_arp();
    test_tables();
    test_new_scales_chords();
    test_rhythm();
    test_page();
    test_human();
    test_rpt();
    printf("smk_clip_keys_test: ok\n");
    return 0;
}
