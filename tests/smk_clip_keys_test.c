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
    static const uint8_t ABS[CK_NCHORD][5] = {
        {0, 4, 7}, {0, 3, 7}, {0, 3, 6}, {0, 4, 8}, {0, 2, 7}, {0, 5, 7}, {0, 4, 7, 11}, {0, 3, 7, 10},
        {0, 4, 7, 10}, {0, 3, 6, 10}, {0, 3, 6, 9}, {0, 4, 7, 11, 14}, {0, 3, 7, 10, 14}, {0, 4, 7, 10, 14},
        {0}, {0}, {0}, {0, 4, 7, 9}, {0, 3, 7, 9}, {0, 3, 7, 11}, {0, 5, 7, 10}, {0, 4, 8, 10}, {0, 4, 7, 14},
        {0, 3, 7, 14}, {0, 7, 11, 14, 17}, {0, 3, 10, 14, 17}, {0, 7, 10, 14, 17}, {0, 4, 11, 14, 21},
        {0, 3, 10, 14, 21}, {0, 4, 10, 14, 21}};
    static const uint8_t SIZE[CK_NCHORD] = {3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 3, 4, 5,
                                            4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5};
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

static void test_zones(void)
{
    uint32_t pg, k, z, n, f, seen;
    for (pg = 0; pg < 3; pg++) {
        uint8_t used[CK_NKEY] = {0};
        const uint16_t *p = ck_zones(pg);
        for (n = seen = 0; p[n]; n++) {
            uint32_t lo = p[n] >> 6 & 63u, hi = p[n] & 63u;
            f = p[n] >> 12;
            assert(f < CK_NE && lo <= hi && hi < CK_NKEY);
            assert(!(seen >> f & 1u));                      /* one zone for each field */
            seen |= 1u << f;
            assert(lo == hi || hi - lo == CK_FMAX[f]);      /* a key for each value (or one key that steps) */
            for (k = lo; k <= hi; k++) {
                assert(!used[k]);                           /* no overlap */
                used[k] = 1;
                assert(ck_zone(pg, k) == p[n]);
            }
        }
        for (k = 0; k < CK_NKEY; k++)
            if (!used[k])
                assert(!ck_zone(pg, k));
    }
    /* the zones of the proposal */
    assert(ck_zone(0, 0) >> 12 == CK_E_ROOT && ck_zone(0, 11) >> 12 == CK_E_ROOT && ck_zone(0, 12) >> 12 == CK_E_SCALE);
    assert(ck_zone(0, 35) >> 12 == CK_E_SCALE && ck_zone(0, 36) >> 12 == CK_E_SNAP);
    assert(ck_zone(1, 29) >> 12 == CK_E_CHORD && !ck_zone(1, 30) && !ck_zone(1, 31));
    assert(ck_zone(1, 32) >> 12 == CK_E_INV && ck_zone(1, 36) >> 12 == CK_E_STRUM);
    assert(ck_zone(2, 5) >> 12 == CK_E_ORD && ck_zone(2, 6) >> 12 == CK_E_OCT && ck_zone(2, 10) >> 12 == CK_E_SWING);
    assert(ck_zone(2, 11) >> 12 == CK_E_HUMAN && ck_zone(2, 12) >> 12 == CK_E_RATE && ck_zone(2, 25) >> 12 == CK_E_GATE);
    assert(ck_zone(2, 35) >> 12 == CK_E_RHY && ck_zone(2, 36) >> 12 == CK_E_LATCH);
    /* the values: a key of a wide zone sets its value; a one-key zone steps and wraps */
    z = ck_zone(0, 14);
    assert(ck_zone_val(z, 14, 0, CK_FMAX[CK_E_SCALE]) == 2);                  /* DORIAN */
    z = ck_zone(2, 12);
    assert(ck_zone_val(z, 21, 0, CK_FMAX[CK_E_RATE]) == 9);                   /* 1/16 */
    z = ck_zone(2, 34);
    assert(ck_zone_val(z, 34, 0, CK_FMAX[CK_E_GATE]) == 9);                   /* 100 % */
    z = ck_zone(1, 36);
    assert(ck_zone_val(z, 36, 3, 7) == 4 && ck_zone_val(z, 36, 7, 7) == 0);   /* STRUM steps, then OFF */
    z = ck_zone(2, 36);
    assert(ck_zone_val(z, 36, 0, 1) == 1 && ck_zone_val(z, 36, 1, 1) == 0);   /* LATCH on, off */
}

/* SCALE CHORD ARP: tap, hold, edit, pin (bits as ck_page_btn takes them) */
static uint32_t tog, t0;
static uint32_t btn(uint32_t pg, uint32_t *prev, uint32_t cur, uint32_t now)
{
    uint32_t press = cur & ~*prev, rel = *prev & ~cur;
    *prev = cur;
    return ck_page_btn(pg, press, rel, cur, now, &t0, &tog);
}
static void test_page(void)
{
    uint32_t pg = 0, prev = 0;
    /* a tap: the stage toggles, the page never shows */
    pg = btn(pg, &prev, 1, 1000);
    assert((pg & 3u) == 1u && !ck_page_vis(pg, 1100 - t0) && !tog);
    pg = btn(pg, &prev, 0, 1200);
    assert(tog == 1u && !(pg & 3u) && !ck_page_vis(pg, 0));
    /* a hold: the page shows after CK_TAP_MS; the release closes it, no toggle */
    pg = btn(pg, &prev, 2, 2000);
    assert(ck_page_vis(pg, 2000 + CK_TAP_MS - t0) && (pg & 3u) == 2u);
    pg = btn(pg, &prev, 0, 2000 + CK_TAP_MS);
    assert(!tog && !(pg & 3u));
    /* a short press with an edit (ck_set sets the field and CK_P_EDIT): no toggle, the page shows at once */
    pg = btn(pg, &prev, 4, 3000);
    pg = (pg & 7u) | CK_P_EDIT | (uint32_t)CK_E_RATE << 4;
    assert(ck_page_vis(pg, 10));
    pg = btn(pg, &prev, 0, 3050);
    assert(!tog && !(pg & 3u) && pg >> 4 == CK_E_RATE);
    /* hold ARP, then FADER BANK: pinned; the release keeps the page; FADER BANK again closes it */
    pg = btn(pg, &prev, 4, 4000);
    pg = btn(pg, &prev, 4 | 16, 4500);
    assert((pg & CK_P_PIN) && (pg & 3u) == 3u);
    pg = btn(pg, &prev, 0, 4600);
    assert(!tog && (pg & 3u) == 3u && ck_page_vis(pg, 4600 - t0));
    /* pinned: a tap on SCALE toggles SCALE and shows its page, still pinned */
    pg = btn(pg, &prev, 1, 5000);
    pg = btn(pg, &prev, 0, 5100);
    assert(tog == 1u && (pg & 3u) == 1u && (pg & CK_P_PIN));
    pg = btn(pg, &prev, 16, 6000);
    assert(!(pg & 3u) && !(pg & CK_P_PIN) && pg >> 4 == CK_E_RATE);
    pg = btn(pg, &prev, 0, 6100);
    /* FADER BANK with no page: the page of the last field (RATE: ARP), pinned */
    pg = btn(pg, &prev, 16, 7000);
    assert((pg & 3u) == 3u && (pg & CK_P_PIN) && ck_page_vis(pg, 0));
    pg = btn(pg, &prev, 0, 7100);
    pg = btn(pg, &prev, 16, 7200);
    assert(!(pg & 3u));
    pg = btn(pg, &prev, 0, 7300);
    pg = 0;                                                 /* last field ROOT: the SCALE page */
    pg = btn(pg, &prev, 16, 7400);
    assert((pg & 3u) == 1u);
    pg = btn(pg, &prev, 0, 7500);
    pg = btn(pg, &prev, 16, 7600);
    pg = btn(pg, &prev, 0, 7700);
    /* NOTE REPEAT + ARP: LATCH, no page; the ARP release is no tap */
    pg = btn(pg, &prev, 8, 8000);
    pg = btn(pg, &prev, 8 | 4, 8100);
    assert(!(pg & 3u));
    pg = btn(pg, &prev, 8, 8200);
    assert(!tog);
    pg = btn(pg, &prev, 0, 8300);
    /* two stages: the second press takes the page; the first release does not close it */
    pg = btn(pg, &prev, 1, 9000);
    pg = btn(pg, &prev, 1 | 2, 9400);
    assert((pg & 3u) == 2u);
    pg = btn(pg, &prev, 2, 9500);
    assert((pg & 3u) == 2u && !tog);
    pg = btn(pg, &prev, 0, 9900);
    assert(!(pg & 3u) && !tog);
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
    test_zones();
    test_page();
    printf("smk_clip_keys_test: ok\n");
    return 0;
}
