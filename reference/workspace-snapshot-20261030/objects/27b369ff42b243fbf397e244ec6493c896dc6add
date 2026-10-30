/* SPDX-License-Identifier: GPL-3.0-only
 * Host test of the clip colour tables and the APC channel states.
 * tests/run_tests.sh first runs tools/apc_pad_colors.py into build/host (apc40_pad_default_gen.h,
 * apc40_lcd565_gen.h). The test compares them with the committed headers, so the tables and
 * the tool cannot drift. tests/apc_palette_test.py checks the colour rules. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define APC40_LCD565 APC40_LCD565_GEN
#include "apc40_lcd565_gen.h"
#undef APC40_LCD565
#define APC40_PAD_DEFAULT APC40_PAD_DEFAULT_GEN
#include "apc40_pad_default_gen.h"
#undef APC40_PAD_DEFAULT
#define PAD_GAMMA_LUT PAD_GAMMA_LUT_GEN
#define PAD_GAIN_Q8 PAD_GAIN_Q8_GEN
#include "apc40_pad_gamma_gen.h"
static const unsigned PAD_FLOOR_LEVEL_GEN = PAD_FLOOR_LEVEL;
#undef PAD_GAMMA_LUT
#undef PAD_GAIN_Q8
#undef PAD_FLOOR_LEVEL
#include "app/apc40_pad_gamma.h"
#include "app/apc40_lcd565.h"
#include "app/apc40_pad_default.h"
#include "app/smk_apc40.h"

static void test_sizes(void)
{
    assert(sizeof APC40_LCD565 == 256u);   /* 128 x RGB565 */
    assert(sizeof APC40_PAD_DEFAULT == 384u);      /* 128 x R, G, B: the calibration start point */
}

static void test_tool_makes_the_same_tables(void)
{
    unsigned i, bad = 0u;
    for (i = 0u; i < 128u; i++) {
        if (APC40_LCD565[i] != APC40_LCD565_GEN[i] || memcmp(APC40_PAD_DEFAULT[i], APC40_PAD_DEFAULT_GEN[i], 3u)) {
            printf("velocity %u: LCD 0x%04X tool 0x%04X, pad %u,%u,%u tool %u,%u,%u\n", i, APC40_LCD565[i],
                   APC40_LCD565_GEN[i], APC40_PAD_DEFAULT[i][0], APC40_PAD_DEFAULT[i][1], APC40_PAD_DEFAULT[i][2],
                   APC40_PAD_DEFAULT_GEN[i][0], APC40_PAD_DEFAULT_GEN[i][1], APC40_PAD_DEFAULT_GEN[i][2]);
            bad++;
        }
    }
    fflush(stdout);
    assert(bad == 0u);
    assert(!memcmp(PAD_GAMMA_LUT, PAD_GAMMA_LUT_GEN, sizeof PAD_GAMMA_LUT));
    assert(!memcmp(PAD_GAIN_Q8, PAD_GAIN_Q8_GEN, sizeof PAD_GAIN_Q8) && PAD_FLOOR_LEVEL == PAD_FLOOR_LEVEL_GEN);
}

/* The same lookups as main_fm1.c (tests/smk_apc40_test.py checks the firmware text). */
static uint16_t lcd_colour(uint8_t v) { return APC40_LCD565[v & 0x7Fu]; }
static uint8_t pad_send(uint8_t v, unsigned ch, uint8_t brightness)
{
    return (uint8_t)(APC40_PAD_DEFAULT[v][ch] * brightness / 510u);
}

static void test_lit_and_empty(void)
{
    unsigned v;
    assert(lcd_colour(0u) == 0u && !(APC40_PAD_DEFAULT[0][0] | APC40_PAD_DEFAULT[0][1] | APC40_PAD_DEFAULT[0][2]));    /* empty cell: dim dot, pad dark */
    for (v = 1u; v < 128u; v++) {
        uint8_t mx = 0u, s;
        assert(lcd_colour((uint8_t)v) != 0u);               /* a lit cell is not black */
        for (s = 0u; s < 3u; s++) {
            uint8_t c = pad_send((uint8_t)v, s, 255u);
            if (c > mx)
                mx = c;
        }
        assert(mx >= 2u);                                   /* the pad floor of the start point: dim entries show */
    }
    for (v = 128u; v < 256u; v++)                           /* > 127 stays in the table */
        assert(lcd_colour((uint8_t)v) == APC40_LCD565[v - 128u]);
}

static void test_channel_states(void)
{
    /* Protocol v1.2 "RGB LEDs Type": channel 0 primary (solid), 1-5 one-shot, 6-10 pulsing,
     * 11-15 blinking, at 1/24, 1/16, 1/8, 1/4, 1/2 (MIDI clock 4, 6, 12, 24, 48 ticks). */
    static const uint8_t ticks[5] = {4u, 6u, 12u, 24u, 48u};
    uint8_t ch;
    for (ch = 0u; ch < 16u; ch++) {
        uint8_t blink = smk_apc40_is_queued(ch);
        assert(blink == (ch >= 11u));                       /* only the blinking states blink */
        if (ch < 6u)
            assert(smk_apc40_animation_clock_period(ch) == 0u);
        else
            assert(smk_apc40_animation_clock_period(ch) == ticks[(ch - 6u) % 5u]);
        /* Solid, one-shot and pulsing draw steady at full level. Blinking is on, then off. */
        assert(smk_apc40_brightness(ch, 0u, 0u) == 255u);
        assert(smk_apc40_brightness(ch, 200u, 0u) == (blink ? 0u : 255u));
    }
    /* The channel does not change the colour: the cell stores the velocity and the channel. */
    {
        volatile uint8_t vel[SMK_APC40_CLIP_COUNT] = {0}, chan[SMK_APC40_CLIP_COUNT] = {0};
        assert(smk_apc40_feedback_set(vel, chan, 5u, 13u, 81u) && vel[5] == 81u && chan[5] == 13u);
        assert(smk_apc40_feedback_set(vel, chan, 5u, 0x1Du, 0xD1u) && vel[5] == 0x51u && chan[5] == 13u);
        assert(!smk_apc40_feedback_set(vel, chan, SMK_APC40_CLIP_COUNT, 0u, 1u));
    }
}

int main(void)
{
    test_sizes();
    test_lit_and_empty();
    test_channel_states();
    test_tool_makes_the_same_tables();
    printf("apc40_tables_test: ok\n");
    return 0;
}
