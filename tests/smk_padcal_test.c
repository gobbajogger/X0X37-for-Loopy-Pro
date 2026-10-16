/* SPDX-License-Identifier: GPL-3.0-only
 * Host test of the pad calibration logic (app/smk_padcal.h, BEHAVIOR.md CLP-79): the page and entry
 * mapping, the knob nudge and its ends, mark ok, and the export word. With an argument it prints an
 * export for tests/smk_padcal_export_test.py (the round trip into tools/apc_pad_colors.py). */
#include <assert.h>
#include <stdio.h>
#include "app/smk_padcal.h"
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
    smk_padcal_start(&c, APC40_PAD_DEFAULT);
    assert(c.on && c.page == 0u && c.sel == 0u);
    for (e = 0; e < 128u; e++) {
        assert(!smk_padcal_is_ok(&c, e));
        assert(smk_padcal_word(&c, e) == ((uint32_t)APC40_PAD_DEFAULT[e][0] | (uint32_t)APC40_PAD_DEFAULT[e][1] << 8 |
                                          (uint32_t)APC40_PAD_DEFAULT[e][2] << 16));
    }
    c.page = 5u;
    c.sel = 1u;                                      /* entry 81 */
    smk_padcal_turn(&c, 2u, -300);                   /* blue to 0, entry ok */
    assert(c.rgb[81][2] == 0u && smk_padcal_is_ok(&c, 81u) && !smk_padcal_is_ok(&c, 80u) && !smk_padcal_is_ok(&c, 82u));
    assert(c.rgb[81][0] == APC40_PAD_DEFAULT[81][0] && c.rgb[81][1] == APC40_PAD_DEFAULT[81][1]);
    assert(APC40_PAD_DEFAULT[81][2] != 0u);         /* the start table does not change */
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

int main(int argc, char **argv)
{
    (void)argv;
    test_mapping();
    test_nudge();
    test_start_turn_mark();
    if (argc > 1) {                                  /* the state above, as the INLOG reply words */
        uint8_t e;
        for (e = 0; e < 128u; e++)
            printf("%u %lu\n", e, (unsigned long)smk_padcal_word(&c, e));
        return 0;
    }
    printf("smk_padcal_test: ok\n");
    return 0;
}
