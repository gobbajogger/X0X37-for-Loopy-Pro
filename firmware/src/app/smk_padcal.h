/* SPDX-License-Identifier: GPL-3.0-only */
/* SMK-37 pad calibration (GLOBAL > SYSTEM > PAD COLOURS, BEHAVIOR.md CLP-79): the logic without hardware.
 * The 16 pads show one page of 16 palette entries (8 pages, 128 entries). The values are pad table
 * levels (R, G, B 0-255, as APC40_RGB): the pads send level / 2, as a lit clip pad. The edits stay in RAM
 * (no settings byte, no flash save). Editor command 38 INLOG, args 2-5, exports 32 entries each.
 * main_fm1.c has the keys, the knobs, the pads and the LCD. tests/smk_padcal_test.c tests this file. */
#ifndef SMK_PADCAL_H
#define SMK_PADCAL_H
#include <stdint.h>
#include <string.h>

#define SMK_PADCAL_PADS 16u
#define SMK_PADCAL_PAGES 8u
#define SMK_PADCAL_CHUNK 32u   /* entries in one export reply */

typedef struct {
    uint8_t on, page, sel;     /* sel: the selected pad (0-15, physical order: top row 0-7) */
    uint8_t ok[16];            /* one bit for each entry: the owner accepted it */
    uint8_t rgb[128][3];
} smk_padcal_t;

static inline void smk_padcal_start(smk_padcal_t *c, const uint8_t def[128][3])
{
    memset(c, 0, sizeof *c);
    memcpy(c->rgb, def, sizeof c->rgb);
    c->on = 1u;
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
/* The export word of an entry: R | G << 8 | B << 16 | ok << 24 (4 x 7 bits in the reply). */
static inline uint32_t smk_padcal_word(const smk_padcal_t *c, uint8_t e)
{
    return (uint32_t)c->rgb[e][0] | (uint32_t)c->rgb[e][1] << 8 | (uint32_t)c->rgb[e][2] << 16 |
           (uint32_t)smk_padcal_is_ok(c, e) << 24;
}
#endif
