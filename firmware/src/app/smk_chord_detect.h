/* SPDX-License-Identifier: GPL-3.0-only
 * Held-note chord recognition, adapted from charlesvestal/fm1-polly:
 * app/dsp/chords.c and chords.h at 62cd07ea7c95c30021c31e957cea8be448415fa7.
 * https://github.com/charlesvestal/fm1-polly
 * X0X changes: compact input summary, no arranger degree tables, eight additional
 * shapes, sharp note spelling, bounded formatting. Pure C; no hardware or state.
 */
#pragma once
#include <stdint.h>

#ifndef CK_CD_FN
#ifdef __clang__
#ifndef X0X_MINSIZE
#if defined(__clang__)
#define X0X_MINSIZE minsize,   /* clang (the firmware toolchain): small code */
#else
#define X0X_MINSIZE            /* gcc host tests: gcc has no minsize */
#endif
#endif
#define CK_CD_FN __attribute__((X0X_MINSIZE noinline, unused))
#else
#define CK_CD_FN __attribute__((noinline, unused))
#endif
#endif

#define CK_CD_NONE 0xFFu
typedef struct {
    uint16_t pcs, upper_pcs;      /* absolute pitch classes, bit 0 = C */
    uint8_t bass, upper_bass;    /* lowest MIDI notes, or CK_CD_NONE */
} ck_chord_input_t;
/* upper excludes only the exact lowest MIDI note, not its pitch class: a bass
 * doubled an octave higher remains in upper_pcs. Duplicate unison notes do not. */
typedef struct {
    uint8_t root, type, bass;    /* root/bass: pitch classes; bass NONE: no slash */
} ck_chord_result_t;

enum {
    CK_CD_MAJ, CK_CD_ADD9, CK_CD_MAJ7, CK_CD_MAJ9, CK_CD_6, CK_CD_AUG,
    CK_CD_7, CK_CD_9, CK_CD_7_S9, CK_CD_7_B9, CK_CD_7_13, CK_CD_7_B13,
    CK_CD_7_S11, CK_CD_AUG7, CK_CD_7SUS4, CK_CD_SUS4, CK_CD_MIN,
    CK_CD_MADD9, CK_CD_MIN7, CK_CD_MIN9, CK_CD_MIN7_11, CK_CD_MIN6,
    CK_CD_MINMAJ7, CK_CD_MIN7_B5, CK_CD_DIM,
    CK_CD_SUS2, CK_CD_DIM7, CK_CD_MAJ11, CK_CD_MIN11, CK_CD_DOM11,
    CK_CD_MAJ13, CK_CD_MIN13, CK_CD_DOM13, CK_CD_NTYPES
};

/* The order retains Polly's tie break. Its partial 11th/13th voicings are kept
 * distinct from X0X's complete stacked 11th/13th shapes. */
static const uint16_t CK_CD_TONES[CK_CD_NTYPES] = {
    0x091, 0x095, 0x891, 0x895, 0x291, 0x111, 0x491, 0x495, 0x499,
    0x493, 0x691, 0x591, 0x4D1, 0x511, 0x4A1, 0x0A1, 0x089, 0x08D,
    0x489, 0x48D, 0x4A9, 0x289, 0x889, 0x449, 0x049,
    0x085, 0x249, 0x8B5, 0x4AD, 0x4B5, 0xAB5, 0x6AD, 0x6B5
};
/* Low nibble: third (or suspended tone); high nibble: fifth. The root is core too. */
static const uint8_t CK_CD_CORE[CK_CD_NTYPES] = {
    0x74, 0x74, 0x74, 0x74, 0x74, 0x84, 0x74, 0x74, 0x74,
    0x74, 0x74, 0x74, 0x74, 0x84, 0x75, 0x75, 0x73, 0x73,
    0x73, 0x73, 0x73, 0x73, 0x73, 0x63, 0x63,
    0x72, 0x63, 0x74, 0x73, 0x74, 0x74, 0x73, 0x74
};
static const char CK_CD_NAMES[] =
    "\0add9\0maj7\0maj9\0" "6\0aug\0" "7\0" "9\0" "7(#9)\0" "7(b9)\0"
    "7(13)\0" "7(b13)\0" "7(#11)\0" "7(#5)\0" "7sus4\0sus4\0m\0madd9\0"
    "m7\0m9\0m7(11)\0m6\0mMaj7\0m7b5\0dim\0sus2\0dim7\0maj11\0m11\0"
    "11\0maj13\0m13\0" "13";
#include "note_names.h"

CK_CD_FN static const char *ck_chord_note_name(uint32_t pc)
{
    /* Same twelve names as UI NOTE_N / clip ROOT (X0X_NOTE_N → X0X_NOTE_PACK). */
    return pc < 12u ? X0X_NOTE_N[pc] : "-";
}

/* At least three bits, without counting the rest. */
CK_CD_FN static uint32_t ck_cd_three(uint32_t mask)
{
    mask &= mask - 1u;
    return mask & (mask - 1u);
}

/* No held tone may be unexplained. Exact matches beat omitted perfect fifths;
 * then a root at the bass wins, then table order. No time/history heuristics.
 * Return root << 8 | type, or -1: no pointer outputs or caller scratch bytes. */
CK_CD_FN static int ck_cd_fit(uint32_t pcs, uint32_t pref)
{
    int best = -1, found = -1;
    uint32_t r, t;
    for (r = 0; r < 12u; r++) {
        uint32_t rel;
        if (!(pcs >> r & 1u))
            continue;
        rel = ((pcs >> r) | (pcs << (12u - r))) & 0xFFFu;
        for (t = 0; t < CK_CD_NTYPES; t++) {
            uint32_t tones = CK_CD_TONES[t], missing = tones & ~rel;
            int score;
            if (rel & ~tones)
                continue;
            if (!missing)
                score = 2000 - (int)t;
            else if (missing == 0x80u)
                score = 1000 - (int)t;
            else
                continue;
            if (r == pref)
                score += 100;
            if (score > best) {
                best = score;
                found = (int)(r << 8 | t);
            }
        }
    }
    return found;
}

/* Return 1 for a chord, 0 for empty/single/dyad/unknown input. On 0 the result
 * is cleared. The caller displays notes for unmatched sets. At least three
 * distinct pitch classes are required, even for an omitted-fifth match.
 * Inversions with a core tone in the bass keep the chord name (E G C -> C).
 * A non-core bass can become a slash under the independently matched upper
 * notes (D below C E G -> C/D). Octave-doubled bass notes prevent that split. */
CK_CD_FN static int ck_chord_detect(const ck_chord_input_t *in, ck_chord_result_t *out)
{
    uint32_t low, iv, core;
    int all, upper, is_core = 0;
    out->root = out->type = out->bass = CK_CD_NONE;
    if (in->bass > 127u || (in->pcs & ~0xFFFu) || (in->upper_pcs & ~in->pcs) ||
        !ck_cd_three(in->pcs))
        return 0;
    low = in->bass % 12u;
    if (!(in->pcs >> low & 1u))
        return 0;
    all = ck_cd_fit(in->pcs, low);
    if (all >= 0) {
        iv = (low + 12u - ((uint32_t)all >> 8)) % 12u;
        core = CK_CD_CORE[(uint32_t)all & 255u];
        is_core = !iv || iv == (core & 15u) || iv == (core >> 4);
    }
    if (!is_core &&
        in->upper_bass <= 127u && in->upper_bass > in->bass &&
        (in->upper_pcs >> (in->upper_bass % 12u) & 1u) &&
        ck_cd_three(in->upper_pcs) && !(in->upper_pcs >> low & 1u) &&
        (upper = ck_cd_fit(in->upper_pcs, in->upper_bass % 12u)) >= 0) {
        all = upper;
        out->bass = (uint8_t)low;
    } else if (all < 0)
        return 0;
    out->root = (uint8_t)((uint32_t)all >> 8);
    out->type = (uint8_t)all;
    return 1;
}

/* end is the last byte available for the terminator. */
CK_CD_FN static char *ck_cd_append(char *out, char *end, const char *s)
{
    while (*s && out < end)
        *out++ = *s++;
    *out = 0;
    return out;
}

/* capacity includes the terminator; capacity 0 writes nothing (out may be NULL).
 * A valid nonzero buffer is always terminated. Invalid results format as "-". */
CK_CD_FN static void ck_chord_format(const ck_chord_result_t *ch, char *out, uint32_t capacity)
{
    const char *name = CK_CD_NAMES;
    char *end;
    uint32_t t;
    if (!capacity)
        return;
    end = out + capacity - 1u;
    *out = 0;
    if (ch->root > 11u || ch->type >= CK_CD_NTYPES || (ch->bass > 11u && ch->bass != CK_CD_NONE)) {
        out = ck_cd_append(out, end, "-");
        return;
    }
    out = ck_cd_append(out, end, ck_chord_note_name(ch->root));
    for (t = 0; t < ch->type; t++)
        while (*name++)
            ;
    out = ck_cd_append(out, end, name);
    if (ch->bass != CK_CD_NONE && ch->bass != ch->root) {
        out = ck_cd_append(out, end, "/");
        out = ck_cd_append(out, end, ck_chord_note_name(ch->bass));
    }
}
