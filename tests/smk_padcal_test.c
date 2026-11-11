/* SPDX-License-Identifier: GPL-3.0-only
 * Host test of the pad calibration logic (app/smk_padcal.h, BEHAVIOR.md CLP-79): the page and entry
 * mapping, the knob nudge and its ends, mark ok, the export word, and the start values (the same as
 * tools/apc_pad_colors.py --pad-h, app/apc40_pad_default.h). With an argument it prints an
 * export for tests/smk_padcal_export_test.py (the round trip into tools/apc_pad_colors.py). */
#include <assert.h>
#include <stdio.h>
#include "app/smk_padcal.h"
#include "app/apc40_lcd565.h"
#include "app/apc40_pad_gamma.h"
#include "app/apc40_pad_default.h"

static smk_padcal_t c;

static void test_mapping(void)
{
    uint8_t p, k, seen[128] = {0};
    for (p = 0; p < SMK_PADCAL_PAGES; p++)
        for (k = 0; k < SMK_PADCAL_PADS; k++) {
            uint8_t e = smk_padcal_entry(p, k);
            assert(e == p * 16u + k && !seen[e]);
            seen[e] = 1u;
        }
    assert(smk_padcal_entry(0, 0) == 0u && smk_padcal_entry(7, 15) == 127u);
    assert(smk_padcal_entry(8, 0) == 0u && smk_padcal_entry(0, 16) == 0u);   /* never out of the table */
    assert(smk_padcal_page_step(0, -1) == 7u && smk_padcal_page_step(7, 1) == 0u);
    assert(smk_padcal_page_step(3, 1) == 4u && smk_padcal_page_step(3, -1) == 2u);
}

static void test_nudge(void)
{
    assert(smk_padcal_nudge(0, -1) == 0u && smk_padcal_nudge(0, -100) == 0u);       /* stops at 0 */
    assert(smk_padcal_nudge(255, 1) == 255u && smk_padcal_nudge(250, 100) == 255u);  /* stops at 255 */
    assert(smk_padcal_nudge(4, 1) == 5u && smk_padcal_nudge(4, -1) == 3u);           /* fine steps when dim */
    assert(smk_padcal_nudge(31, 1) == 32u && smk_padcal_nudge(32, 1) == 34u);
    assert(smk_padcal_nudge(200, -1) == 193u && smk_padcal_nudge(1, -3) == 0u);
}

static void test_start_turn_mark(void)
{
    uint8_t e;
    smk_padcal_start(&c, APC40_LCD565, PAD_GAMMA_LUT, PAD_GAIN_Q8, PAD_FLOOR_LEVEL);
    assert(c.on && c.page == 0u && c.sel == 0u);
    for (e = 0; e < 128u; e++) {
        if (c.rgb[e][0] != APC40_PAD_DEFAULT[e][0] || c.rgb[e][1] != APC40_PAD_DEFAULT[e][1] ||
            c.rgb[e][2] != APC40_PAD_DEFAULT[e][2])
            printf("entry %u: firmware %u,%u,%u, tool %u,%u,%u\n", e, c.rgb[e][0], c.rgb[e][1], c.rgb[e][2],
                   APC40_PAD_DEFAULT[e][0], APC40_PAD_DEFAULT[e][1], APC40_PAD_DEFAULT[e][2]);
        assert(!smk_padcal_is_ok(&c, e));
        assert(smk_padcal_word(&c, e) == ((uint32_t)APC40_PAD_DEFAULT[e][0] | (uint32_t)APC40_PAD_DEFAULT[e][1] << 8 |
                                          (uint32_t)APC40_PAD_DEFAULT[e][2] << 16));
    }
    c.page = 5u;
    c.sel = 1u;                                      /* entry 81 */
    smk_padcal_turn(&c, 2u, -300);                   /* blue to 0, entry ok */
    assert(c.rgb[81][2] == 0u && smk_padcal_is_ok(&c, 81u) && !smk_padcal_is_ok(&c, 80u) && !smk_padcal_is_ok(&c, 82u));
    assert(c.rgb[81][0] == APC40_PAD_DEFAULT[81][0] && c.rgb[81][1] == APC40_PAD_DEFAULT[81][1]);
    smk_padcal_mark(&c);                             /* PLAY: not ok again, the value stays */
    assert(!smk_padcal_is_ok(&c, 81u) && c.rgb[81][2] == 0u);
    smk_padcal_mark(&c);
    assert(smk_padcal_is_ok(&c, 81u));
    assert(smk_padcal_word(&c, 81u) >> 24 == 1u);
    c.page = 7u;
    c.sel = 15u;                                     /* entry 127, the last bit */
    smk_padcal_mark(&c);
    assert(smk_padcal_is_ok(&c, 127u) && c.ok[15] == 0x80u);
}

/* The orientation page (CLP-79): no knob turn + confirm = identity; each correction is one-to-one, and the
 * four corrections of a 2 x 8 grid put the entries where expected. */
static void test_orientation(void)
{
    static const uint8_t TL[4] = {0u, 8u, 7u, 15u};   /* as assumed, rows swapped, rows mirrored, 180 degrees */
    uint8_t t, i;
    smk_padcal_start(&c, APC40_LCD565, PAD_GAMMA_LUT, PAD_GAIN_Q8, PAD_FLOOR_LEVEL);
    assert(c.orient == 1u && c.tl == 0u);
    c.orient = 0u;                                   /* SEQ PLAY with no knob turn */
    for (i = 0; i < 16u; i++)
        assert(smk_padcal_phys(c.tl, i) == i);       /* identity */
    smk_padcal_orient_turn(&c, -1);
    assert(c.tl == 15u);                             /* K2 back from pad 1: wrap to the last pad */
    smk_padcal_orient_turn(&c, 3);
    assert(c.tl == 2u);
    for (t = 0; t < 16u; t++) {
        uint16_t seen = 0u;
        for (i = 0; i < 16u; i++) {
            uint8_t ph = smk_padcal_phys(t, i);
            assert(ph < 16u && smk_padcal_phys(t, ph) == i);   /* the way back (a pad press) */
            seen |= (uint16_t)(1u << ph);
        }
        assert(seen == 0xFFFFu && smk_padcal_phys(t, 0u) == t);   /* one-to-one; position 0 is the chosen pad */
    }
    for (t = 0; t < 4u; t++)
        for (i = 0; i < 16u; i++) {
            uint8_t row = i >> 3, col = i & 7u, ph = smk_padcal_phys(TL[t], i);
            uint8_t er = (TL[t] & 8u) ? 1u - row : row, ec = (TL[t] & 7u) ? 7u - col : col;
            assert(ph == er * 8u + ec);
        }
}

/* A second open of PAD COLOURS (CLP-79): the edits, the ok marks and the orientation correction stay until
 * power off. Only the first open after power-on computes the start values. The orientation page lights the
 * pad of the saved correction. */
static void test_reopen(void)
{
    static smk_padcal_t r;                           /* .bss: as after power-on */
    smk_padcal_start(&r, APC40_LCD565, PAD_GAMMA_LUT, PAD_GAIN_Q8, PAD_FLOOR_LEVEL);
    smk_padcal_orient_turn(&r, 7);                   /* K2: rows mirrored */
    r.orient = 0u;                                   /* SEQ PLAY */
    r.page = 5u;
    r.sel = 1u;                                      /* entry 81 */
    smk_padcal_turn(&r, 0u, 40);                     /* red up, entry ok */
    r.page = 2u;
    r.sel = 3u;                                      /* entry 35 */
    smk_padcal_mark(&r);
    {
        uint8_t keep81[3] = {r.rgb[81][0], r.rgb[81][1], r.rgb[81][2]};
        assert(keep81[0] != APC40_PAD_DEFAULT[81][0]);
        r.on = 0u;                                   /* PATCH: the mode ends */
        smk_padcal_start(&r, APC40_LCD565, PAD_GAMMA_LUT, PAD_GAIN_Q8, PAD_FLOOR_LEVEL);
        assert(r.on == 1u && r.orient == 1u && r.page == 0u && r.sel == 0u);
        assert(r.rgb[81][0] == keep81[0] && r.rgb[81][1] == keep81[1] && r.rgb[81][2] == keep81[2]);
        assert(smk_padcal_is_ok(&r, 81u) && smk_padcal_is_ok(&r, 35u) && !smk_padcal_is_ok(&r, 80u));
        assert(r.tl == 7u && smk_padcal_phys(r.tl, 0u) == 7u);   /* the lit pad: the saved top-left */
        assert(r.rgb[80][0] == APC40_PAD_DEFAULT[80][0] && r.rgb[80][2] == APC40_PAD_DEFAULT[80][2]);
    }
}

/* The ok dot on the LCD (CLP-79) must contrast with its square for every entry. The luminance of an RGB565
 * colour (Rec. 601 weights 77 / 150 / 29 of 256, each channel taken to 8 bits) is 0-255. The limit is 128,
 * half the scale: black on white (or white on black) is 255, and a dot of one fixed colour gives 0 on a
 * square of that colour (entry 3, white). With black on a square of luminance >= 128 and white below it,
 * the difference is never less than 128. */
static uint32_t luma565(uint32_t c)
{
    uint32_t r = (c >> 11 & 31u) * 255u / 31u, g = (c >> 5 & 63u) * 255u / 63u, b = (c & 31u) * 255u / 31u;
    return (77u * r + 150u * g + 29u * b) >> 8;
}
static void test_dot_contrast(void)
{
    uint32_t e, bad = 0u;
    for (e = 0; e < 128u; e++) {
        uint32_t sq = luma565(APC40_LCD565[e]), dot = luma565(smk_padcal_dot565(APC40_LCD565[e]));
        uint32_t diff = sq > dot ? sq - dot : dot - sq;
        if (diff < 128u) {
            if (bad++ < 4u)
                printf("entry %lu: square luminance %lu, dot luminance %lu\n", (unsigned long)e, (unsigned long)sq,
                       (unsigned long)dot);
        }
    }
    if (bad) {
        printf("%lu entries with a dot of low contrast\n", (unsigned long)bad);
        fflush(stdout);
    }
    assert(bad == 0u);
}

int main(int argc, char **argv)
{
    (void)argv;
    test_mapping();
    test_nudge();
    test_orientation();
    test_start_turn_mark();
    test_reopen();
    test_dot_contrast();
    c.tl = 8u;                                       /* the export carries the correction */
    c.on = 0u;                                       /* leave and open again: the export keeps the edits */
    smk_padcal_start(&c, APC40_LCD565, PAD_GAMMA_LUT, PAD_GAIN_Q8, PAD_FLOOR_LEVEL);
    if (argc > 1) {                                  /* the state above, as the INLOG reply words */
        uint8_t e;
        for (e = 0; e < 128u; e++)
            printf("%u %lu\n", e, (unsigned long)smk_padcal_word(&c, e));
        printf("tl %u\n", c.tl);
        return 0;
    }
    printf("smk_padcal_test: ok\n");
    return 0;
}
