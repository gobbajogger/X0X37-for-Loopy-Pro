/* SPDX-License-Identifier: GPL-3.0-only
 * Host test of the LCD clip colour table (firmware/src/app/apc40_lcd565.h).
 * tests/run_tests.sh first runs tools/apc_pad_colors.py --c into build/host/apc40_lcd565_gen.h.
 * The test compares the two tables, so the table and the tool cannot drift.
 * It also checks the entries that the owner approved (docs/USER_PREFERENCES.md). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* The table that the tool makes now, under a second name. */
#define APC40_LCD565 APC40_LCD565_GEN
#include "apc40_lcd565_gen.h"
#undef APC40_LCD565
/* The table that the firmware will use. */
#include "app/apc40_lcd565.h"

static void test_size(void)
{
    /* 128 velocities, 2 bytes each: 256 B of constant data in flash. */
    assert(sizeof APC40_LCD565 / sizeof APC40_LCD565[0] == 128u);
    assert(sizeof APC40_LCD565 == 256u);
}

static void test_tool_makes_the_same_table(void)
{
    unsigned i, bad = 0u;
    for (i = 0u; i < 128u; i++) {
        if (APC40_LCD565[i] != APC40_LCD565_GEN[i]) {
            printf("velocity 0x%02X: header 0x%04X, tool 0x%04X\n", i, APC40_LCD565[i],
                   APC40_LCD565_GEN[i]);
            bad++;
        }
    }
    fflush(stdout);
    assert(bad == 0u);
}

static void test_approved_entries(void)
{
    /* Velocity 0 is an empty cell: black, the firmware draws a dim dot. */
    assert(APC40_LCD565[0x00] == 0x0000u);
    assert(APC40_LCD565[0x03] == 0xFFFFu);   /* white */
    assert(APC40_LCD565[0x05] == 0xFA06u);   /* red full (the owner likes this red) */
    assert(APC40_LCD565[0x15] == 0x07E0u);   /* green full */
    assert(APC40_LCD565[0x2D] == 0x543Eu);   /* blue full: no drift toward purple */
    assert(APC40_LCD565[0x30] == 0x939Eu);
    assert(APC40_LCD565[0x31] == 0x7BFEu);   /* violet full */
    assert(APC40_LCD565[0x32] == 0x325Du);   /* strong hue fix (owner approved) */
    assert(APC40_LCD565[0x33] == 0x3A17u);   /* strong hue fix (owner approved) */
    assert(APC40_LCD565[0x7F] == 0xA160u);   /* the highest velocity */
}

static void test_lit_cells_are_not_black(void)
{
    unsigned i;
    /* Each velocity 1-127 is a lit cell. A black lit cell looks the same as an empty cell. */
    for (i = 1u; i < 128u; i++)
        assert(APC40_LCD565[i] != 0x0000u);
}

/* The same lookup as smk_clip_lcd_draw (tests/smk_apc40_test.py checks the firmware text). */
static uint16_t lcd_colour(uint8_t v)
{
    return APC40_LCD565[v & 0x7Fu];
}

static void test_lookup_edges(void)
{
    unsigned v;
    assert(lcd_colour(0u) == 0x0000u);      /* empty cell: the firmware draws a dim dot */
    assert(lcd_colour(1u) == APC40_LCD565[1]);
    assert(lcd_colour(127u) == 0xA160u);    /* the highest velocity */
    /* A value above 127 cannot come from MIDI (smk_apc_usb_event masks with 0x7F).
     * If it comes, the lookup stays inside the table. */
    for (v = 128u; v < 256u; v++)
        assert(lcd_colour((uint8_t)v) == APC40_LCD565[v - 128u]);
}

int main(void)
{
    test_size();
    test_approved_entries();
    test_lit_cells_are_not_black();
    test_lookup_edges();
    test_tool_makes_the_same_table();
    printf("apc40_lcd565_test: ok\n");
    return 0;
}
