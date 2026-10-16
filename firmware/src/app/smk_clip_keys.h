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
#define CK_NSCALE 11u
#define CK_NCHORD 17u
#define CK_NORD 6u
#define CK_NRATE 13u
enum { CK_M_SCALE = 1u, CK_M_CHORD = 2u, CK_M_ARP = 4u };
enum { CK_ORD_UP, CK_ORD_DOWN, CK_ORD_UPDN, CK_ORD_DNUP, CK_ORD_PLAYED, CK_ORD_RANDOM };

/* 12-bit pitch masks, bit 0 = the root */
static const uint16_t CK_SCALE_MASK[CK_NSCALE] = {0xAB5, 0x5AD, 0x6AD, 0x5AB, 0xAD5, 0x6B5, 0x56B,
                                                  0x9AD, 0xAAD, 0x295, 0x4A9};
static const char CK_SCALE_N[] = "MAJOR\0MINOR\0DORIAN\0PHRYG\0LYDIAN\0MIXO\0LOCRIAN\0HARM MIN\0MEL MIN\0MAJ PENT\0MIN PENT";
/* chords: the intervals above the root, one nibble each, low nibble first; 0 ends the list.
 * SC types (14-16): one nibble for each scale third stacked on the root (2, 3 or 4 thirds) */
static const uint16_t CK_CHORD_T[CK_NCHORD] = {0x74, 0x73, 0x63, 0x84, 0x72, 0x75, 0xB74, 0xA73, 0xA74, 0xA63,
                                               0x963, 0xEB74, 0xEA73, 0xEA74, 0x22, 0x222, 0x2222};
static const char CK_CHORD_N[] = "MAJ\0MIN\0DIM\0AUG\0SUS2\0SUS4\0MAJ7\0MIN7\0DOM7\0M7B5\0DIM7\0MAJ9\0MIN9\0DOM9\0"
                                 "SC TRIAD\0SC 7TH\0SC 9TH";
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
            v = note + (t & 15u);
            if (type >= 14)                          /* SC types: the second scale note above the last note */
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
