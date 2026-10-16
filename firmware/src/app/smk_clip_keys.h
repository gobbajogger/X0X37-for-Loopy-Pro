/* SPDX-License-Identifier: GPL-3.0-only
 * Clip mode keybed: scale, chord and arp logic (SMK-37). Pure functions, no hardware access.
 * main_fm1.c owns the state, the MIDI output and the controls. tests/smk_clip_keys_test.c tests this file.
 * All settings are 0-based and in range (project_load checks them): no modulo on a setting. */
#pragma once
#include <stdint.h>

#ifndef CK_FN
#define CK_FN __attribute__((minsize, noinline, unused))   /* small code: the app area is almost full */
#endif

#define CK_NKEY 37u           /* keybed keys; key 0 = C3 = MIDI note 48 */
#define CK_BASE_NOTE 48
#define CK_MAXN 5u            /* the most notes of one chord */
#define CK_ARPN 24u           /* the most notes in one arp cycle (before the octaves) */
#define CK_NSCALE 24u
#define CK_NCHORD 30u
#define CK_NRHY 16u
#define CK_NORD 6u
#define CK_NRATE 13u
enum { CK_M_SCALE = 1u, CK_M_CHORD = 2u, CK_M_ARP = 4u };
enum { CK_ORD_UP, CK_ORD_DOWN, CK_ORD_UPDN, CK_ORD_DNUP, CK_ORD_PLAYED, CK_ORD_RANDOM };

/* 12-bit pitch masks, bit 0 = the root */
static const uint16_t CK_SCALE_MASK[CK_NSCALE] = {0xAB5, 0x5AD, 0x6AD, 0x5AB, 0xAD5, 0x6B5, 0x56B, 0x9AD,
                                                  0xAAD, 0x295, 0x4A9, 0xFFF, 0x4E9, 0x29D, 0x9B5, 0x5B3,
                                                  0x6D5, 0x55B, 0x555, 0x6DB, 0xB6D, 0xEB5, 0x18D, 0x9B3};
static const char CK_SCALE_N[] = "MAJOR\0MINOR\0DORIAN\0PHRYG\0LYDIAN\0MIXO\0LOCRIAN\0HARM MIN\0MEL MIN\0MAJ PENT\0MIN PENT\0"
                                 "CHROMATIC\0BLUES\0MAJ BLUES\0HARM MAJ\0PHRYG DOM\0LYD DOM\0ALTERED\0WHOLE TONE\0"
                                 "DIM H-W\0DIM W-H\0BEBOP DOM\0HIRAJOSHI\0DBL HARM";
/* chords: the step from the last chord note to the next, one nibble each, low nibble first; 0 ends the list.
 * SC types (14-16): one nibble for each scale third stacked on the root (2, 3 or 4 thirds) */
static const uint16_t CK_CHORD_T[CK_NCHORD] = {0x34, 0x43, 0x33, 0x44, 0x52, 0x25, 0x434, 0x343, 0x334, 0x433,
                                               0x333, 0x3434, 0x4343, 0x4334, 0x22, 0x222, 0x2222, 0x234,
                                               0x243, 0x443, 0x325, 0x244, 0x734, 0x743, 0x3347, 0x3473,
                                               0x3437, 0x7374, 0x7473, 0x7464};
static const char CK_CHORD_N[] = "MAJ\0MIN\0DIM\0AUG\0SUS2\0SUS4\0MAJ7\0MIN7\0DOM7\0M7B5\0DIM7\0MAJ9\0MIN9\0DOM9\0"
                                 "SC TRIAD\0SC 7TH\0SC 9TH\0MAJ6\0MIN6\0MMAJ7\0" "7SUS4\0AUG7\0ADD9\0MADD9\0"
                                 "MAJ11\0MIN11\0DOM11\0MAJ13\0MIN13\0DOM13";
/* arp rhythms: (hits << 4) | (steps - 1); 0 = OFF (every step plays). The hits are spread evenly (Euclid). */
static const uint8_t CK_RHY_T[CK_NRHY] = {0x00, 0x22, 0x33, 0x24, 0x34, 0x36, 0x46, 0x56,
                                         0x37, 0x57, 0x77, 0x48, 0x58, 0x5B, 0x7B, 0x5F};
static const char CK_RHY_N[] = "OFF\0" "2/3\0" "3/4\0" "2/5\0" "3/5\0" "3/7\0" "4/7\0" "5/7\0" "3/8\0" "5/8\0" "7/8\0"
                               "4/9\0" "5/9\0" "5/12\0" "7/12\0" "5/16";
static const char CK_ORD_N[] = "UP\0DOWN\0UP/DN\0DN/UP\0PLAYED\0RANDOM";
/* arp rates in MIDI clock ticks (24 for each quarter note) */
static const uint8_t CK_RATE_T[CK_NRATE] = {48, 36, 32, 24, 18, 16, 12, 9, 8, 6, 4, 3, 2};
static const char CK_RATE_N[] = "1/2\0" "1/4D\0" "1/2T\0" "1/4\0" "1/8D\0" "1/4T\0" "1/8\0" "1/16D\0" "1/8T\0" "1/16\0"
                                "1/16T\0" "1/32\0" "1/32T";
static const char CK_ROOT_N[] = "C\0C#\0D\0D#\0E\0F\0F#\0G\0G#\0A\0A#\0B";

/* The name number n in a list of names, each one ends with a 0 byte */
CK_FN static const char *ck_nm(const char *s, uint32_t n)
{
    while (n--)
        while (*s++)
            ;
    return s;
}

/* The scale as 12 bits of absolute pitch classes (bit 0 = C) */
CK_FN static uint32_t ck_mask(uint32_t scale, uint32_t root)
{
    uint32_t m = (uint32_t)CK_SCALE_MASK[scale] << root;
    return (m | m >> 12) & 0xFFFu;
}

/* 1 if the note is in the scale m (ck_mask) */
CK_FN static uint32_t ck_in(uint32_t m, uint32_t note)
{
    return (m >> (note % 12u)) & 1u;
}

/* The notes of one key: scale snap, then the chord, then the inversion. m: ck_mask (also for the SC chords).
 * snap: 0 off, 1 NEAR (the nearest scale note, a tie goes down), 2 FILTER (no note out of the scale).
 * type: the chord, or -1 for none. Return the count (0..CK_MAXN). The input note is 48..84: all notes stay
 * in 0..127. */
CK_FN static uint32_t ck_notes(uint32_t note, uint32_t m, uint32_t snap, int32_t type, uint32_t inv, uint8_t *out)
{
    uint32_t n = 1, i, t, d, v;
    if (snap && !ck_in(m, note)) {
        if (snap > 1u)
            return 0;
        for (d = 1;; d++) {
            if (ck_in(m, note - d)) {
                note -= d;
                break;
            }
            if (ck_in(m, note + d)) {
                note += d;
                break;
            }
        }
    }
    out[0] = (uint8_t)note;
    if (type >= 0)
        for (t = CK_CHORD_T[type]; t; t >>= 4) {
            v = out[n - 1u] + (t & 15u);
            if ((uint32_t)type - 14u < 3u)                          /* SC types: the second scale note above the last note */
                for (v = out[n - 1u], d = 2; d;) {
                    v++;
                    d -= ck_in(m, v);
                }
            out[n++] = (uint8_t)v;
        }
    for (i = 0; i < inv && i + 1u < n; i++)          /* inversion: the lowest notes go up one octave */
        out[i] = (uint8_t)(out[i] + 12u);
    return n;
}

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

/* 1 if the arp step s plays in rhythm r (CK_RHY_T index). 0 = OFF: every step plays. */
CK_FN static uint32_t ck_rhy_hit(uint32_t r, uint32_t s)
{
    uint32_t l = (CK_RHY_T[r] & 15u) + 1u, h = CK_RHY_T[r] >> 4;
    return !r || (s % l) * h % l < h;
}
/* The arp note number (ck_arp_pick step) of arp step s in rhythm r: the count of the hits before it */
CK_FN static uint32_t ck_rhy_pos(uint32_t r, uint32_t s)
{
    uint32_t l = (CK_RHY_T[r] & 15u) + 1u, h = CK_RHY_T[r] >> 4, i = s % l;
    return r ? s / l * h + (i ? (i - 1u) * h / l + 1u : 0u) : s;
}

/* Clip keys editor: the key zones of one page. Each zone is a range of keys that sets one field. A one-key
 * zone steps its field by one (and wraps). Fields: */
enum { CK_E_ROOT, CK_E_SCALE, CK_E_SNAP, CK_E_CHORD, CK_E_INV, CK_E_STRUM, CK_E_ORD, CK_E_RATE, CK_E_OCT,
       CK_E_GATE, CK_E_SWING, CK_E_HUMAN, CK_E_RHY, CK_E_LATCH, CK_NE };
/* the highest value of each field */
static const uint8_t CK_FMAX[CK_NE] = {11, CK_NSCALE - 1, 1, CK_NCHORD - 1, 3, 7, CK_NORD - 1, CK_NRATE - 1, 3, 9, 7, 7,
                                       CK_NRHY - 1, 1};
/* zone: (field << 12) | (first key << 6) | last key, in key order; 3 pages (SCALE, CHORD, ARP) */
static const uint16_t CK_ZONE[] = {
    CK_E_ROOT << 12 | 0 << 6 | 11, CK_E_SCALE << 12 | 12 << 6 | 35, CK_E_SNAP << 12 | 36 << 6 | 36, 0,
    CK_E_CHORD << 12 | 0 << 6 | 29, CK_E_INV << 12 | 32 << 6 | 35, CK_E_STRUM << 12 | 36 << 6 | 36, 0,
    CK_E_ORD << 12 | 0 << 6 | 5, CK_E_OCT << 12 | 6 << 6 | 9, CK_E_SWING << 12 | 10 << 6 | 10,
    CK_E_HUMAN << 12 | 11 << 6 | 11, CK_E_RATE << 12 | 12 << 6 | 24, CK_E_GATE << 12 | 25 << 6 | 34,
    CK_E_RHY << 12 | 35 << 6 | 35, CK_E_LATCH << 12 | 36 << 6 | 36, 0};
/* The first zone of page pg (0 SCALE, 1 CHORD, 2 ARP) */
CK_FN static const uint16_t *ck_zones(uint32_t pg)
{
    const uint16_t *z = CK_ZONE;
    while (pg)
        pg -= !*z++;
    return z;
}
/* The zone of key k on page pg: (field << 12 | first << 6 | last), or 0 for a key with no zone */
CK_FN static uint32_t ck_zone(uint32_t pg, uint32_t k)
{
    const uint16_t *z;
    for (z = ck_zones(pg); *z; z++)
        if (k >= (*z >> 6 & 63u) && k <= (*z & 63u))
            return *z;
    return 0;
}
/* The new value of a field after key k in zone z. v: the old value, max: the highest value. */
CK_FN static uint32_t ck_zone_val(uint32_t z, uint32_t k, uint32_t v, uint32_t max)
{
    uint32_t lo = z >> 6 & 63u;
    if (lo == (z & 63u))
        return v >= max ? 0u : v + 1u;
    k -= lo;
    return k > max ? max : k;
}

/* Clip keys page state (one byte): bits 0-1 the page (0 none, 1 SCALE, 2 CHORD, 3 ARP), bit 2 pinned, bit 3 an
 * edit since the last stage press, bits 4-7 the last field (CK_E_*) */
enum { CK_P_PIN = 4u, CK_P_EDIT = 8u };
#define CK_TAP_MS 300u        /* a shorter stage press with no edit is a tap; a longer one shows the page */
/* 1: the page is on the screen. dt: the ms since the last stage press. */
CK_FN static uint32_t ck_page_vis(uint32_t pg, uint32_t dt)
{
    return (pg & 3u) && ((pg & (CK_P_PIN | CK_P_EDIT)) || dt >= CK_TAP_MS);
}
/* The stage buttons and FADER BANK. press, rel: the buttons that went down, up; cur: the buttons down now
 * (bits 0-2 SCALE CHORD ARP, bit 3 NOTE REPEAT, bit 4 FADER BANK). A stage press opens its page (held);
 * NOTE REPEAT + ARP is LATCH (no page). A stage release on its page: a tap (no edit, shorter than CK_TAP_MS)
 * puts the stage bit in *tog; the page closes unless pinned. FADER BANK pins the page (no page: the page of
 * the last field), a second press closes it. now: ms; *t0: the time of the last stage press. Return the
 * new page state. */
CK_FN static uint32_t ck_page_btn(uint32_t pg, uint32_t press, uint32_t rel, uint32_t cur, uint32_t now,
                                  uint32_t *t0, uint32_t *tog)
{
    uint32_t i, f;
    *tog = 0;
    for (i = 0; i < 3u; i++) {
        if ((press >> i & 1u) && !(i == 2u && (cur & 8u))) {
            *t0 = now;
            pg = (pg & (0xF0u | CK_P_PIN)) | (i + 1u);
        }
        if ((rel >> i & 1u) && (pg & 3u) == i + 1u) {
            if (!(pg & CK_P_EDIT) && now - *t0 < CK_TAP_MS)
                *tog |= 1u << i;
            if (!(pg & CK_P_PIN))
                pg &= 0xF0u;
        }
    }
    f = pg >> 4;
    if (press & 16u)
        pg = pg & CK_P_PIN ? pg & 0xF0u
                           : (pg & 3u ? pg : pg | (f < CK_E_CHORD ? 1u : f < CK_E_ORD ? 2u : 3u)) | CK_P_PIN | CK_P_EDIT;
    return pg;
}
