/* HOST SIZE PROBE only. Not a firmware patch. Not linked into the device image.
 * Same JieLi flags as ui.c (-Os, -ffunction-sections, -fdata-sections, -fno-builtin,
 * -mcpu=r3, minsize, noinline). put_s and NOTE_N are extern: they already live in ui.c.
 *
 * Tier 1 required; Tier 2 included when SEQ_CHORD_TIER2 is 1.
 * Matching rules are the final notation spec: bass-first exact mask, first match
 * wins if the bass is the root; otherwise a single non-bass match slashes, two
 * or more non-bass matches (or none) return 0 and the caller shows notes.
 */
#include <stdint.h>

char *put_s(char *b, const char *s);
extern const char *const NOTE_N[12];

#ifndef SEQ_CHORD_TIER2
#define SEQ_CHORD_TIER2 0
#endif

static const uint16_t CH_MASK[] = {
    0x0091u, /* maj    {0,4,7}      bare root */
    0x0089u, /* min    {0,3,7}      min       */
    0x0081u, /* 5      {0,7}        5         */
    0x0491u, /* 7      {0,4,7,10}   7         */
    0x0891u, /* maj7   {0,4,7,11}   maj7      */
    0x0489u, /* min7   {0,3,7,10}   min7      */
#if SEQ_CHORD_TIER2
    0x0049u, /* dim    {0,3,6}      dim       */
    0x00A1u, /* sus4   {0,5,7}      sus4      */
    0x0085u, /* sus2   {0,2,7}      sus2      */
    0x0111u, /* aug    {0,4,8}      aug       */
    0x0291u, /* 6      {0,4,7,9}    6         */
    0x0449u, /* min7b5 {0,3,6,10}   min7b5    */
    0x0249u, /* dim7   {0,3,6,9}    dim7      */
#endif
};
#define CH_N ((int)(sizeof CH_MASK / sizeof CH_MASK[0]))

static const char CH_SUF[] =
    "\0min\0" "5\0" "7\0" "maj7\0" "min7"
#if SEQ_CHORD_TIER2
    "\0dim\0sus4\0sus2\0aug\0" "6\0min7b5\0dim7"
#endif
    ;

/* 1: wrote Root+suffix[/Bass] (no octave). 0: show notes; dst is empty. */
__attribute__((minsize, noinline))
int seq_chord_symbol(char *dst, const uint8_t *notes, int n)
{
    uint16_t pcs = 0;
    int i, r, t, nfit = 0;
    uint8_t bass, fit_root = 0, fit_type = 0;
    const char *s;

    if (n <= 0) {
        dst[0] = 0;
        return 0;
    }
    for (i = 0; i < n; i++)
        pcs |= (uint16_t)(1u << (notes[i] % 12u));
    bass = (uint8_t)(notes[0] % 12u);

    for (r = 0; r < 12; r++) {
        uint8_t pc = (uint8_t)((unsigned)(bass + r) % 12u);
        uint16_t rel;
        if (!(pcs & (uint16_t)(1u << pc)))
            continue;
        rel = (uint16_t)(((pcs >> pc) | (pcs << (12 - pc))) & 0xFFFu);
        for (t = 0; t < CH_N; t++) {
            if (rel != CH_MASK[t])
                continue;
            if (pc == bass) {
                fit_root = pc;
                fit_type = (uint8_t)t;
                goto format;
            }
            if (nfit == 0) {
                fit_root = pc;
                fit_type = (uint8_t)t;
            }
            nfit++;
            break;
        }
    }
    if (nfit != 1) {
        dst[0] = 0;
        return 0;
    }

format:
    dst = put_s(dst, NOTE_N[fit_root]);
    s = CH_SUF;
    for (t = 0; t < (int)fit_type; t++)
        while (*s++)
            ;
    dst = put_s(dst, s);
    if (fit_root != bass) {
        *dst++ = '/';
        dst = put_s(dst, NOTE_N[bass]);
    }
    *dst = 0;
    return 1;
}
