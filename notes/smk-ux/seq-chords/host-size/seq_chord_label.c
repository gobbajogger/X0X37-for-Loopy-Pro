/* HOST SIZE PROBE for the §10 label add-on. Not a firmware patch.
 * Not linked into the device image. Same JieLi flags as seq_chord_name.c.
 *
 * This file measures the ADD-ON only: TIER3 data, an interval formatter,
 * a nearest-tag walker, and a label wrapper that writes a primary and
 * an optional second string. seq_chord_symbol, put_s and NOTE_N are
 * extern (already in ui.c / the §9 object). The exact-match walk used
 * by the second-reading hook is included here as seq_chord_walk so its
 * incremental text can be seen; Firmware Main Builder can fold the
 * stash into the existing seq_chord_symbol loop instead, which is
 * cheaper than a second walk.
 *
 * SEQ_CHORD_OPT:  A=second-reading only, B=T3+nearest+interval,
 *                 C=interval only, D=A+colon, R=B+A (recommended).
 */
#include <stdint.h>

char *put_s(char *b, const char *s);
extern const char *const NOTE_N[12];

#ifndef SEQ_CHORD_OPT
#define SEQ_CHORD_OPT 'R'
#endif

#if SEQ_CHORD_OPT == 'B' || SEQ_CHORD_OPT == 'R'
static const uint16_t T3_MASK[] = {
    0x0015u, /* add2    {0,2,4}        */
    0x0095u, /* add9    {0,2,4,7}      */
    0x008Du, /* min+9   {0,2,3,7}      */
    0x04A1u, /* 7sus4   {0,5,7,10}     */
    0x0495u, /* 9       {0,2,4,7,10}   */
    0x048Du, /* min9    {0,2,3,7,10}   */
    0x0895u, /* maj9    {0,2,4,7,11}   */
    0x04B5u, /* 11      {0,2,4,5,7,10} */
    0x04ADu, /* min11   {0,2,3,5,7,10} */
    0x08B5u, /* maj11   {0,2,4,5,7,11} */
    0x0295u, /* 69      {0,2,4,7,9}    */
    0x0889u, /* minmaj7 {0,3,7,11}     */
};
#define T3_N ((int)(sizeof T3_MASK / sizeof T3_MASK[0]))
static const char T3_SUF[] =
    "add2\0add9\0min+9\07sus4\09\0min9\0maj9\0"
    "11\0min11\0maj11\069\0minmaj7";
#endif

#if SEQ_CHORD_OPT == 'B' || SEQ_CHORD_OPT == 'C' || SEQ_CHORD_OPT == 'R'
/* Degree names 1..11, each NUL-terminated, in semitone order. */
static const char DEG[] = "b2\0" "2\0" "b3\0" "3\0" "4\0" "#4\0" "5\0" "#5\0" "6\0" "b7\0" "7";

__attribute__((minsize, noinline))
int seq_interval(char *dst, const uint8_t *notes, int n)
{
    uint16_t pcs = 0;
    int i, d, npc = 0;
    uint8_t bass;
    const char *s;
    if (n <= 0) { dst[0] = 0; return 0; }
    for (i = 0; i < n; i++)
        pcs |= (uint16_t)(1u << (notes[i] % 12u));
    for (i = 0; i < 12; i++)
        if (pcs & (uint16_t)(1u << i)) npc++;
    bass = (uint8_t)(notes[0] % 12u);
    dst = put_s(dst, NOTE_N[bass]);
    if (npc == 1) {
        /* octave double */
        dst = put_s(dst, "oct");
        *dst = 0;
        return 1;
    }
    *dst++ = '(';
    for (d = 1; d < 12; d++) {
        uint8_t pc = (uint8_t)((unsigned)(bass + d) % 12u);
        if (!(pcs & (uint16_t)(1u << pc)))
            continue;
        s = DEG;
        for (i = 1; i < d; i++)
            while (*s++)
                ;
        if (*(dst - 1) != '(')
            *dst++ = ' ';
        dst = put_s(dst, s);
    }
    *dst++ = ')';
    *dst = 0;
    return 1;
}
#endif

#if SEQ_CHORD_OPT == 'B' || SEQ_CHORD_OPT == 'R'
/* Unique pure extension from the bass against T3 AND against two extern
 * tables for T1/T2, passed as pointer+length so we do not duplicate
 * CH_MASK. For this probe we only walk T3_MASK plus a tiny caller-supplied
 * table pointer (extern). A 0 return means "not unique / none". */
extern const uint16_t CH_MASK[];
extern const char CH_SUF[];
extern const int CH_N;

__attribute__((minsize, noinline))
int seq_nearest(char *dst, const uint8_t *notes, int n)
{
    uint16_t pcs = 0, rel, extra, miss;
    int i, t, best_n = 99, ties = 0, best_t = -1, n_ex;
    uint8_t bass;
    const char *s;
    if (n <= 0) { dst[0] = 0; return 0; }
    for (i = 0; i < n; i++)
        pcs |= (uint16_t)(1u << (notes[i] % 12u));
    bass = (uint8_t)(notes[0] % 12u);
    rel = (uint16_t)(((pcs >> bass) | (pcs << (12 - bass))) & 0xFFFu);

    /* Walk T1+T2 (extern) then T3. Encoded: t >= 100 means T3 index t-100. */
    for (t = 0; t < CH_N + T3_N; t++) {
        uint16_t mask = (t < CH_N) ? CH_MASK[t] : T3_MASK[t - CH_N];
        extra = (uint16_t)(rel & ~mask);
        miss  = (uint16_t)(mask & ~rel);
        if (miss != 0 || extra == 0)
            continue;
        n_ex = 0;
        for (i = 0; i < 12; i++)
            if (extra & (uint16_t)(1u << i)) n_ex++;
        if (n_ex < best_n) {
            best_n = n_ex;
            ties = 1;
            best_t = t;
        } else if (n_ex == best_n) {
            ties++;
        }
    }
    if (ties != 1 || best_t < 0) {
        dst[0] = 0;
        return 0;
    }
    dst = put_s(dst, NOTE_N[bass]);
    if (best_t < CH_N) {
        s = CH_SUF;
        for (t = 0; t < best_t; t++)
            while (*s++)
                ;
        dst = put_s(dst, s);
        extra = (uint16_t)(rel & ~CH_MASK[best_t]);
    } else {
        s = T3_SUF;
        for (t = 0; t < best_t - CH_N; t++)
            while (*s++)
                ;
        dst = put_s(dst, s);
        extra = (uint16_t)(rel & ~T3_MASK[best_t - CH_N]);
    }
    for (i = 1; i < 12; i++) {
        if (extra & (uint16_t)(1u << i)) {
            *dst++ = '+';
            dst = put_s(dst, NOTE_N[(bass + i) % 12u]);
        }
    }
    *dst = 0;
    return 1;
}
#endif

#if SEQ_CHORD_OPT == 'A' || SEQ_CHORD_OPT == 'D' || SEQ_CHORD_OPT == 'R'
/* Write "Root+suf[/Bass]" for one already-identified match. Small helper
 * so the second-reading format does not redo the matcher. */
__attribute__((minsize, noinline))
char *seq_put_reading(char *dst, uint8_t root, const char *suf, uint8_t bass)
{
    dst = put_s(dst, NOTE_N[root]);
    dst = put_s(dst, suf);
    if (root != bass) {
        *dst++ = '/';
        dst = put_s(dst, NOTE_N[bass]);
    }
    *dst = 0;
    return dst;
}
#endif

/* Label wrapper: given a primary already in `pri` (from seq_chord_symbol
 * or a fallback) and an optional second root/suffix, write `sec`.
 * For D, the caller appends ':' on the NOTE cell, not here. */
#if SEQ_CHORD_OPT == 'A' || SEQ_CHORD_OPT == 'D' || SEQ_CHORD_OPT == 'R'
__attribute__((minsize, noinline))
int seq_put_second(char *sec, uint8_t root, const char *suf, uint8_t bass, int have)
{
    if (!have) {
        sec[0] = 0;
        return 0;
    }
    seq_put_reading(sec, root, suf, bass);
    return 1;
}
#endif
