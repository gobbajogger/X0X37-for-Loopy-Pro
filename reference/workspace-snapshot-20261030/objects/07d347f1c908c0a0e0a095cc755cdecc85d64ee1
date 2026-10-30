/* SPDX-License-Identifier: GPL-3.0-only */
/* SMK-37 pad calibration (GLOBAL > SYSTEM > PAD COLOURS, BEHAVIOR.md CLP-79): the logic without hardware.
 * The 16 pads show one page of 16 palette entries (8 pages, 128 entries). The values are pad table
 * levels (R, G, B 0-255, as APC40_PAD_OWNER): the pads send level / 2, as a lit clip pad. The edits stay in RAM
 * (no settings byte, no flash save). Editor command 38 INLOG, args 2-5, exports 32 entries each.
 * main_fm1.c has the keys, the knobs, the pads and the LCD. tests/smk_padcal_test.c tests this file. */
#ifndef SMK_PADCAL_H
#define SMK_PADCAL_H
#include <stdint.h>


#define SMK_PADCAL_PADS 16u
#define SMK_PADCAL_PAGES 8u
#define SMK_PADCAL_CHUNK 32u   /* entries in one export reply */

typedef struct {
    uint8_t on, page, sel;     /* sel: the selected pad (0-15, reading order: top row 0-7, left to right) */
    uint8_t orient, tl;        /* orient: the orientation page is on; tl: the pad index the owner found top-left */
    uint8_t init;              /* the start values are set (first open after power-on) */
    uint8_t real;              /* SEQ REC: the selected LCD square at its real level (no boost) */
    uint8_t ok[16];            /* one bit for each entry: the owner accepted it */
    uint8_t rgb[128][3];
} smk_padcal_t;

/* The start values of one entry from its LCD colour (RGB565, the Akai colour): the gamma table (5-bit
 * index; green uses its 5 high bits), the gains (Q8), then the scale back to the brightest decoded
 * channel. Below floor: the 5-bit codes scaled to floor (the hue stays). tools/apc_colour_model.py
 * pad_default does the same steps; app/apc40_pad_gamma.h has the constants. */
static inline void smk_padcal_default(uint8_t out[3], uint32_t c565, const uint8_t lut[32], const uint16_t gain[3],
                                      uint32_t floor)
{
    uint32_t q[3], lin[3], d[3], k, ml = 0u, md = 0u, mx = 0u, mq = 0u;
    q[0] = c565 >> 11;
    q[1] = (c565 >> 6) & 31u;
    q[2] = c565 & 31u;
    for (k = 0; k < 3u; k++) {
        lin[k] = lut[q[k]];
        d[k] = lin[k] * gain[k];
        ml = lin[k] > ml ? lin[k] : ml;
        md = d[k] > md ? d[k] : md;
        mq = q[k] > mq ? q[k] : mq;
    }
    for (k = 0; k < 3u; k++) {
        d[k] = md ? d[k] * ml / md : 0u;
        mx = d[k] > mx ? d[k] : mx;
    }
    for (k = 0; k < 3u; k++) {
        if (mx < floor)
            d[k] = mq ? q[k] * floor / mq : 0u;
        out[k] = (uint8_t)d[k];               /* d <= the brightest decoded channel (255), or <= floor */
    }
}
static inline void smk_padcal_start(smk_padcal_t *c, const uint16_t lcd565[128], const uint8_t lut[32],
                                    const uint16_t gain[3], uint32_t floor)
{
    uint32_t e;
    if (!c->init) {                                /* only the first open after power-on: the edits stay */
        for (e = 0; e < 128u; e++)
            smk_padcal_default(c->rgb[e], lcd565[e], lut, gain, floor);
        __builtin_memset(c->ok, 0, sizeof c->ok);
        c->tl = 0u;
        c->init = 1u;
    }
    c->page = c->sel = 0u;
    c->on = c->orient = 1u;                        /* the orientation page lights the pad of the saved tl */
}
/* The colour of the ok dot on the LCD square of colour c565 (RGB565): black on a light square (luminance
 * 128-255, Rec. 601 weights 77 / 150 / 29 of 256), white on a dark one. The difference is then always 128 or
 * more (tests/smk_padcal_test.c test_dot_contrast). */
static inline uint16_t smk_padcal_dot565(uint32_t c565)
{
    uint32_t y = 77u * ((c565 >> 11 & 31u) * 255u / 31u) + 150u * ((c565 >> 5 & 63u) * 255u / 63u) +
                 29u * ((c565 & 31u) * 255u / 31u);
    return (y >> 8) >= 128u ? 0x0000u : 0xFFFFu;
}
/* The boost of the selected LCD square (owner 2026-10-17 01:42, 01:43): the hue of a dark entry is hard to
 * see, so the LCD square of the selected entry shows 2^s x brighter (each RGB565 field << s). s is the
 * largest value 0-3 that keeps each field in its range. The pads and the stored values do not change. */
static inline uint8_t smk_padcal_boost_shift(uint32_t c565)
{
    uint32_t r = c565 >> 11 & 31u, g = c565 >> 5 & 63u, b = c565 & 31u, s = 0u;
    while (s < 3u && (r << (s + 1u)) <= 31u && (g << (s + 1u)) <= 63u && (b << (s + 1u)) <= 31u)
        s++;
    return (uint8_t)(r | g | b ? s : 0u);
}
static inline uint16_t smk_padcal_boost565(uint32_t c565, uint8_t s)
{
    return (uint16_t)((c565 >> 11 & 31u) << s << 11 | (c565 >> 5 & 63u) << s << 5 | (c565 & 31u) << s);
}
/* The orientation page: the pad index 0 is lit; K2 moves the light (wrap); SEQ PLAY confirms it as top-left.
 * The pad index of reading position i is i ^ tl (and back: the same). The model: the pad index of the
 * firmware is a 2 x 8 grid whose row order and column order can be wrong. tl 0 = as assumed, 8 = rows
 * swapped, 7 = each row mirrored, 15 = both (180 degrees). The other values are also one-to-one. */
static inline uint8_t smk_padcal_phys(uint8_t tl, uint8_t i)
{
    return (uint8_t)((i ^ tl) & 15u);
}
static inline void smk_padcal_orient_turn(smk_padcal_t *c, int32_t d)
{
    c->tl = (uint8_t)((c->tl + (uint32_t)d) & 15u);
}
/* The entry of a pad on the page: page 0 pads 0-15 = entries 0-15, ..., page 7 = 112-127. */
static inline uint8_t smk_padcal_entry(uint8_t page, uint8_t pad)
{
    return (uint8_t)((page % SMK_PADCAL_PAGES) * SMK_PADCAL_PADS + pad % SMK_PADCAL_PADS);
}
/* OCT- / OCT+: one page down or up, with wrap. */
static inline uint8_t smk_padcal_page_step(uint8_t page, int32_t d)
{
    return (uint8_t)((page + (uint32_t)d) % SMK_PADCAL_PAGES);
}
/* One knob turn of d detents: one level a detent below 32, more above (1 + level / 32). Stops at 0 and 255. */
static inline uint8_t smk_padcal_nudge(uint8_t v, int32_t d)
{
    int32_t n = v + d * (int32_t)(1u + (v >> 5));
    return (uint8_t)(n < 0 ? 0 : n > 255 ? 255 : n);
}
static inline uint8_t smk_padcal_is_ok(const smk_padcal_t *c, uint8_t e)
{
    return (uint8_t)((c->ok[e >> 3] >> (e & 7u)) & 1u);
}
/* A knob turn on the selected pad: the channel ch (0 R, 1 G, 2 B) changes and the entry is ok. */
static inline void smk_padcal_turn(smk_padcal_t *c, uint8_t ch, int32_t d)
{
    uint8_t e = smk_padcal_entry(c->page, c->sel);
    c->rgb[e][ch] = smk_padcal_nudge(c->rgb[e][ch], d);
    c->ok[e >> 3] |= (uint8_t)(1u << (e & 7u));
}
/* PLAY: the selected entry ok, or not ok again. */
static inline void smk_padcal_mark(smk_padcal_t *c)
{
    uint8_t e = smk_padcal_entry(c->page, c->sel);
    c->ok[e >> 3] ^= (uint8_t)(1u << (e & 7u));
}
/* One entry as a word: R | G << 8 | B << 16 | ok << 24 (the LCD redraw key; tests/smk_padcal_test.c prints it). */
static inline uint32_t smk_padcal_word(const smk_padcal_t *c, uint8_t e)
{
    return (uint32_t)c->rgb[e][0] | (uint32_t)c->rgb[e][1] << 8 | (uint32_t)c->rgb[e][2] << 16 |
           (uint32_t)smk_padcal_is_ok(c, e) << 24;
}
#endif
