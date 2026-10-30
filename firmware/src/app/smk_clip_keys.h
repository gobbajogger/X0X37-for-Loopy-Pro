/* SPDX-License-Identifier: GPL-3.0-only
 * Clip mode keybed: scale, chord and arp logic (SMK-37). Pure functions, no hardware access.
 * main_fm1.c owns the state, the MIDI output and the controls. tests/smk_clip_keys_test.c tests this file.
 * All settings are 0-based and in range (project_load checks them): no modulo on a setting. */
#pragma once
#include <stdint.h>

#ifndef CK_FN
#ifndef X0X_MINSIZE
#if defined(__clang__)
#define X0X_MINSIZE minsize,   /* clang (the firmware toolchain): small code */
#else
#define X0X_MINSIZE            /* gcc host tests: gcc has no minsize */
#endif
#endif
#define CK_FN __attribute__((X0X_MINSIZE noinline, unused))   /* small code: the app area is almost full */
#endif

#define CK_NKEY 37u           /* keybed keys; key 0 = C3 = MIDI note 48 */
#define CK_BASE_NOTE 48
#define CK_MAXN 7u            /* the most notes of one chord (the 13th chords) */
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
static const uint32_t CK_CHORD_T[CK_NCHORD] = {0x34, 0x43, 0x33, 0x44, 0x52, 0x25, 0x434, 0x343,
                                               0x334, 0x433, 0x333, 0x3434, 0x4343, 0x4334, 0x22,
                                               0x222, 0x2222, 0x234, 0x243, 0x443, 0x325, 0x244,
                                               0x734, 0x743, 0x33434, 0x34343, 0x34334, 0x433434,
                                               0x434343, 0x434334};
static const char CK_CHORD_N[] = "MAJ\0MIN\0DIM\0AUG\0SUS2\0SUS4\0MAJ7\0MIN7\0DOM7\0M7B5\0DIM7\0MAJ9\0MIN9\0DOM9\0"
                                 "SC TRIAD\0SC 7TH\0SC 9TH\0MAJ6\0MIN6\0MMAJ7\0" "7SUS4\0AUG7\0ADD9\0MADD9\0"
                                 "MAJ11\0MIN11\0DOM11\0MAJ13\0MIN13\0DOM13";
/* arp rhythms: (hits << 4) | (steps - 1); 0 = OFF (every step plays). The hits are spread evenly (Euclid). */
static const uint8_t CK_RHY_T[CK_NRHY] = {0x00, 0x22, 0x33, 0x24, 0x34, 0x36, 0x46, 0x56,
                                         0x37, 0x57, 0x77, 0x48, 0x58, 0x5B, 0x7B, 0x5F};
static const char CK_RHY_N[] = "OFF\0" "2/3\0" "3/4\0" "2/5\0" "3/5\0" "3/7\0" "4/7\0" "5/7\0" "3/8\0" "5/8\0" "7/8\0"
                               "4/9\0" "5/9\0" "5/12\0" "7/12\0" "5/16";
static const char CK_ORD_N[] = "UP\0DOWN\0UP/DN\0DN/UP\0PLAYED\0RANDOM";
/* arp rates in MIDI clock ticks (24 for each quarter note).
 * Table + ck_arp_pick live in smk_clip_arp_share.c (one copy for engine.o + x0x.o). */
extern const uint8_t CK_RATE_T[CK_NRATE];
static const char CK_RATE_N[] = "1/2\0" "1/4D\0" "1/2T\0" "1/4\0" "1/8D\0" "1/4T\0" "1/8\0" "1/16D\0" "1/8T\0" "1/16\0"
                                "1/16T\0" "1/32\0" "1/32T";
#include "note_names.h"
#define CK_ROOT_N X0X_NOTE_PACK          /* same twelve names as the UI note labels */

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
 * oct: 1..4. rnd: a random number (RANDOM only). Return -1 for a note above 127.
 * Defined once in smk_clip_arp_share.c (Chicago Arp + clip MIDI). */
int ck_arp_pick(const uint8_t *s, uint32_t n, uint32_t order, uint32_t oct, uint32_t step,
                uint32_t rnd);

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

/* Fields selected with knob 2 on a stage page. */
enum { CK_E_ROOT, CK_E_SCALE, CK_E_SNAP, CK_E_CHORD, CK_E_INV, CK_E_STRUM, CK_E_ORD, CK_E_RATE, CK_E_OCT,
       CK_E_GATE, CK_E_SWING, CK_E_HUMAN, CK_E_RHY, CK_E_LATCH, CK_E_ON, CK_NE };
/* The highest value of each field. */
static const uint8_t CK_FMAX[CK_NE] = {11, CK_NSCALE - 1, 1, CK_NCHORD - 1, 3, 7, CK_NORD - 1, CK_NRATE - 1, 3, 9, 7, 7,
                                       CK_NRHY - 1, 1, 1};

/* Clip keys page state: bits 0-1 page (0 none, 1 SCALE, 2 CHORD, 3 ARP), bits 4-7 selected field. */
/* The ON field is first on each page, followed by the fields shown on the screen. */
static const uint8_t CK_PAGE_FIELDS[3][9] = {
    {CK_E_ON, CK_E_ROOT, CK_E_SCALE, CK_E_SNAP, CK_NE},
    {CK_E_ON, CK_E_CHORD, CK_E_INV, CK_E_STRUM, CK_NE},
    {CK_E_ON, CK_E_ORD, CK_E_OCT, CK_E_SWING, CK_E_HUMAN, CK_E_RATE, CK_E_GATE, CK_E_RHY, CK_E_LATCH}};
CK_FN static uint32_t ck_page_field(uint32_t pg, uint32_t f, int32_t d)
{
    const uint8_t *a = CK_PAGE_FIELDS[pg - 1u];
    uint32_t i, n = pg == 3u ? 9u : 4u;
    for (i = 0; i < n && a[i] != f; i++)
        ;
    if (i == n)
        i = 0;
    return a[(uint32_t)((int32_t)i + (d > 0 ? 1 : (int32_t)n - 1)) % n];
}
/* A press opens its page immediately; pressing the open page again closes it. */
CK_FN static uint32_t ck_page_press(uint32_t pg, uint32_t press, uint32_t cur)
{
    uint32_t i;
    for (i = 0; i < 3u; i++)
        if ((press >> i & 1u) && !(i == 2u && (cur & 8u)))
            pg = (pg & 3u) == i + 1u ? 0u : (CK_E_ON << 4) | (i + 1u);
    return pg;
}

/* < and > on a page: a press steps the last field at once. A hold steps again after CK_RPT_DELAY ms, then
 * each CK_RPT_MS ms. Only an arrow that went down on a page with no menu steps or repeats. */
#define CK_RPT_DELAY 400u
#define CK_RPT_MS 80u
#define CK_RPT_ARM (1u << 16)
/* lr: < and > down now (bit 0 <, bit 1 >), 0 with no page or a menu; press: the bits that went down; now: ms;
 * *s: bits 0-15 the ms of the next repeat, CK_RPT_ARM: the held arrow went down while lr allowed it.
 * Return -1 (<), +1 (>) or 0. None or both held: no step, and the arrow that stays down needs a new press. */
CK_FN static int32_t ck_rpt(uint32_t lr, uint32_t press, uint32_t now, uint32_t *s)
{
    if (!lr || lr == 3u) {
        *s = 0;
        return 0;
    }
    if (press & lr)
        *s = CK_RPT_ARM | (uint16_t)(now + CK_RPT_DELAY);
    else if ((*s & CK_RPT_ARM) && (int16_t)(uint16_t)(now - *s) >= 0)
        *s = CK_RPT_ARM | (uint16_t)(now + CK_RPT_MS);
    else
        return 0;
    return lr == 2u ? 1 : -1;
}

/* The value of a field after one step st (-1 or +1, from ck_rpt) from v: 0..max, no wrap */
CK_FN static uint32_t ck_step(uint32_t v, int32_t st, uint32_t max)
{
    return st > 0 ? (v < max ? v + 1u : v) : (v ? v - 1u : 0u);
}

/* HUMAN 0-7. r: a random number. Return the velocity: vel moved by up to 4 x h at random (1..127); *late: the
 * ms the note-on waits, 0 to 4 x h (CK_LATE_MAX at most). h = 0: vel, no wait. */
#define CK_LATE_MAX 28u
CK_FN static uint32_t ck_human(uint32_t h, uint32_t r, uint32_t vel, uint32_t *late)
{
    int32_t v = (int32_t)vel + (int32_t)((r >> 24) - 128u) * (int32_t)h / 32;
    *late = (r >> 8 & 255u) * (4u * h + 1u) >> 8;
    return (uint32_t)(v < 1 ? 1 : v > 127 ? 127 : v);
}
/* 1: a waiting note-on with send time t (the low 8 bits of the ms) is due at now. A note waits at most
 * CK_LATE_MAX ms: a t more than that ahead of now means a long main loop pass (late, not early). */
CK_FN static uint32_t ck_q_due(uint32_t t, uint32_t now)
{
    return (uint8_t)(t - now - 1u) >= CK_LATE_MAX;
}
