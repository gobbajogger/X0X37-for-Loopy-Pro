/* SPDX-License-Identifier: GPL-3.0-only
 * Clip mode keybed: scale, chord and arp logic (SMK-37). Pure functions, no hardware access.
 * main_fm1.c owns the state, the MIDI output and the controls. tests/smk_clip_keys_test.c tests this file. */
#pragma once
#include <stdint.h>

#ifndef CK_FN
#define CK_FN __attribute__((minsize, noinline, unused))   /* small code: the app area is almost full */
#endif

#define CK_NKEY 37u           /* keybed keys; key 0 = C3 = MIDI note 48 */
#define CK_BASE_NOTE 48
#define CK_MAXN 5u            /* the most notes of one chord */
#define CK_HELD 16u           /* the most notes in the arp list */
#define CK_NSCALE 11u
#define CK_NCHORD 14u
#define CK_NORD 5u
#define CK_NRATE 13u
enum { CK_M_SCALE = 1u, CK_M_CHORD = 2u, CK_M_ARP = 4u };
enum { CK_ORD_UP, CK_ORD_DOWN, CK_ORD_UPDN, CK_ORD_PLAYED, CK_ORD_RANDOM };

/* 12-bit pitch masks, bit 0 = the root */
static const uint16_t CK_SCALE_MASK[CK_NSCALE] = {0xAB5, 0x5AD, 0x6AD, 0x5AB, 0xAD5, 0x6B5, 0x56B,
                                                  0x9AD, 0xAAD, 0x295, 0x4A9};
static const char CK_SCALE_N[] = "MAJOR\0MINOR\0DORIAN\0PHRYG\0LYDIAN\0MIXO\0LOCRIAN\0HARM MIN\0MEL MIN\0MAJ PENT\0MIN PENT";
/* chords: the note count, then the intervals above the root */
static const uint8_t CK_CHORD_T[CK_NCHORD][CK_MAXN] = {
    {3, 4, 7}, {3, 3, 7}, {3, 3, 6}, {3, 4, 8}, {3, 2, 7}, {3, 5, 7},
    {4, 4, 7, 11}, {4, 3, 7, 10}, {4, 4, 7, 10}, {4, 3, 6, 10}, {4, 3, 6, 9},
    {5, 4, 7, 11, 14}, {5, 3, 7, 10, 14}, {5, 4, 7, 10, 14}};
static const char CK_CHORD_N[] = "MAJ\0MIN\0DIM\0AUG\0SUS2\0SUS4\0MAJ7\0MIN7\0DOM7\0M7B5\0DIM7\0MAJ9\0MIN9\0DOM9";
static const char CK_ORD_N[] = "UP\0DOWN\0UP/DN\0PLAYED\0RANDOM";
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

CK_FN static int ck_pc(int note, int root)
{
    int pc = (note - root) % 12;
    return pc < 0 ? pc + 12 : pc;
}

/* Snap a note to the nearest scale note. A tie goes down. */
CK_FN static int ck_snap(int note, int root, uint16_t mask)
{
    int pc = ck_pc(note, root), d;
    if ((mask >> pc) & 1u)
        return note;
    for (d = 1; d < 12; d++) {
        if ((mask >> ((pc + 12 - d) % 12)) & 1u)
            return note - d >= 0 ? note - d : note + 12 - d;
        if ((mask >> ((pc + d) % 12)) & 1u)
            return note + d <= 127 ? note + d : note + d - 12;
    }
    return note;
}

/* Make the chord notes of a root note. Return the count. Notes above 127 are dropped. */
CK_FN static uint32_t ck_chord(int note, uint32_t type, uint8_t *out)
{
    const uint8_t *t = CK_CHORD_T[type < CK_NCHORD ? type : 0];
    uint32_t n = t[0], i, k = 0;
    int v[CK_MAXN];
    v[0] = note;
    for (i = 1; i < n; i++)
        v[i] = note + t[i];
    for (i = 0; i < n; i++)
        if (v[i] >= 0 && v[i] <= 127)
            out[k++] = (uint8_t)v[i];
    return k;
}

/* The arp note for a step. notes: the list in press order. oct: 1..4. Return -1 for none. */
CK_FN static int ck_arp_pick(const uint8_t *notes, uint32_t n, uint32_t order, uint32_t oct, uint32_t step)
{
    uint8_t s[CK_HELD];
    uint32_t len, i, j;
    int v;
    if (!n)
        return -1;                                   /* (n <= CK_HELD: the caller) */
    for (i = 0; i < n; i++) {
        uint8_t x = notes[i];
        j = i;
        if (order != CK_ORD_PLAYED)
            for (; j > 0 && s[j - 1] > x; j--)
                s[j] = s[j - 1];
        s[j] = x;
    }
    len = n * oct;                                   /* (oct 1-4: project_load) */
    if (order == CK_ORD_UPDN) {
        j = len > 1u ? step % (2u * len - 2u) : 0u;
        i = j < len ? j : 2u * len - 2u - j;
    } else if (order == CK_ORD_DOWN) {
        i = len - 1u - step % len;
    } else if (order == CK_ORD_RANDOM) {
        i = (step * 2654435761u >> 13) % len;    /* a fixed scramble of the step: no state */
    } else {
        i = step % len;
    }
    v = s[i % n] + 12 * (int)(i / n);
    return v <= 127 ? v : -1;
}

/* Gate length in ticks: rate ticks x gate (1..10 = 10..100 %), at least 1 tick */
CK_FN static uint32_t ck_gate_ticks(uint32_t rate_ticks, uint32_t gate10)
{
    uint32_t g = rate_ticks * gate10 / 10u;
    return g ? g : 1u;
}
