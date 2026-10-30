/* SPDX-License-Identifier: GPL-3.0-only
 * #57/#58: the current HOME 8-column design must not lose later steps or
 * let quiet overlapping notes hide the strongest event. No hardware needed.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/home_pattern_glyph.h"

static void clear(pattern_t *pt, lane_t lanes[NLANE])
{
    memset(pt, 0, sizeof *pt);
    memset(lanes, 0, NLANE * sizeof *lanes);
    for (int k = 0; k < NKIT; k++) pt->drum[k].len = 16;
    for (int b = 0; b < NBASS; b++) pt->bass[b].len = 16;
}
static void test_drums(void)
{
    pattern_t pt; lane_t lanes[NLANE]; uint8_t a[8];
    clear(&pt, lanes);
    home_pattern_strength8(&pt, lanes, 0, 0, 88, 8, a);
    for (int k = 0; k < 8; k++) assert(a[k] == 0);
    sm_set(pt.drum[0].hit[0], 0);
    sm_set(pt.drum[0].hit[2], 1);
    sm_set(pt.drum[0].accent, 1);
    home_pattern_strength8(&pt, lanes, 0, 0, 88, 8, a);
    assert(a[0] == 127); /* overlapping unaccented 88 must not hide accented 127 */
    pt.drum[0].len = 64;
    sm_set(pt.drum[0].hit[1], 63);
    home_pattern_strength8(&pt, lanes, 0, 0, 88, 8, a);
    assert(a[0] == 127 && a[7] == 88); /* the final 48 steps are not discarded */
    lanes[0].used = 1; lanes[0].pat = 0; lanes[0].part = 0;
    lanes[0].t = 8; lanes[0].v = 1; lanes[0].i = 0;
    memset(lanes[0].val, MOT_NONE, sizeof lanes[0].val);
    lanes[0].val[63] = 5;
    home_pattern_strength8(&pt, lanes, 0, 0, 88, 8, a);
    assert(a[7] == 5); /* actual stored note velocity controls brightness */
    lanes[0].val[63] = 115;
    lanes[1] = lanes[0]; lanes[1].val[63] = 1; /* duplicate/corrupt lane cannot mask the first */
    sm_set(pt.drum[0].hit[10], 63); /* quieter/stronger sounds in same bin */
    home_pattern_strength8(&pt, lanes, 0, 0, 88, 8, a);
    assert(a[7] == 115); /* the stronger of two hits wins */
    lanes[0].pat = 1;
    home_pattern_strength8(&pt, lanes, 0, 0, 88, 8, a);
    assert(a[7] == 88); /* another pattern's automation must not bleed in */
    pt.drum[1].len = 64; sm_set(pt.drum[1].hit[2], 48);
    home_pattern_strength8(&pt, lanes, 0, 1, 88, 8, a);
    assert(a[6] == 88 && a[7] == 0); /* the 808 has a distinct grid */
}
static void test_bass_and_break(void)
{
    pattern_t pt; lane_t lanes[NLANE]; uint8_t a[8];
    clear(&pt, lanes);
    bpart_t *b = &pt.bass[0];
    b->len = 64;
    b->step[16].flags = G_NOTE; b->step[16].velocity = 120;
    b->step[17].flags = G_TIE;
    b->step[18].flags = G_REST;
    home_pattern_strength8(&pt, lanes, 0, 2, 88, 8, a);
    assert(a[2] == 120 && a[0] == 0 && a[3] == 0);
    memset(b->step, 0, sizeof b->step);
    b->step[0].flags = G_TIE; b->step[1].flags = G_TIE;
    b->step[63].flags = G_NOTE; b->step[63].velocity = 23;
    home_pattern_strength8(&pt, lanes, 0, 2, 88, 8, a);
    assert(a[0] == 23 && a[7] == 23); /* valid wrap, low velocity preserved */
    memset(b->step, 0, sizeof b->step);
    for (int s = 0; s < 64; s++) b->step[s].flags = G_TIE;
    home_pattern_strength8(&pt, lanes, 0, 2, 88, 8, a);
    for (int k = 0; k < 8; k++) assert(a[k] == 0); /* no phantom all-TIE note */
    pt.brk.steps = 1u << 15;
    home_pattern_strength8(&pt, lanes, 0, 4, 88, 8, a);
    assert(a[7] == 127 && a[0] == 0);
}
int main(void)
{
    test_drums(); test_bass_and_break();
    for (int current = 0; current <= 1; current++) {
        uint8_t previous = home_glyph_brightness(1, current);
        assert(home_glyph_brightness(0, current) == 0);
        assert(previous > 0);
        for (int v = 2; v <= 127; v++) {
            uint8_t n = home_glyph_brightness((uint8_t)v, current);
            assert(n >= previous && n <= 16);
            previous = n;
        }
    }
    assert(home_glyph_brightness(88, 1) > home_glyph_brightness(88, 0));
    assert(home_glyph_brightness(127, 1) > home_glyph_brightness(20, 1));
    puts("PASS home_pattern_glyph: full 16/64 steps, separate kits, max velocity, wrap/orphan ties, breaks");
    return 0;
}
