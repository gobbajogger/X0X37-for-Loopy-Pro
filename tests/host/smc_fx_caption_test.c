/* SPDX-License-Identifier: GPL-3.0-only */
/* FX-page SMC button-caption contract, independent of private DSP headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app/smc_fx_caption.h"

static unsigned checks;
#define CHECK(x) do { checks++; if (!(x)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); \
    return 1; \
} } while (0)

int main(void)
{
    static const char *const tap[SMCFX_N] = {
        "LPF", "LPF", "LPF", "LPF", "LPF", "RVB", "DLY", "MIX"
    };
    for (int bus = 0; bus < SMCFX_N; bus++) {
        const char *name = smcfx_select_caption(bus, 0, 0, SMCFX_NFX);
        CHECK(strcmp(name, tap[bus]) == 0);
        CHECK(!strcmp(smcfx_select_caption(bus, 0, 1, SMCFX_NFX), "LPF")); /* momentary hold */
        CHECK(strlen(name) <= 3u);   /* 30-pixel physical strip */
        CHECK(!strcmp(smcfx_select_caption(bus, 1, 0, SMCFX_NFX), "CYC"));
        CHECK(smcfx_is_main(bus) == (bus >= SMCFX_RVB));
        CHECK(smcfx_bus(SMCFX_NOTE[bus]) == bus); /* keep the physical strip */
        for (int page = 0; page < SMCFX_NPAGE; page++)
            CHECK(smcfx_suit_ok(bus, SMCFX_PAGE[page][bus]));
    }
    /* The old caption advertised the assigned throw, rather than the action
     * of a tap; the existing ROOM preset makes that mismatch reproducible. */
    CHECK(strcmp(SMCFX_SHORT[SMCFX_PAGE[0][SMCFX_909]],
                 smcfx_select_caption(SMCFX_909, 0, 0, SMCFX_NFX)) != 0);
    CHECK(strcmp(SMCFX_SHORT[SMCFX_PAGE[0][SMCFX_RVB]],
                 smcfx_select_caption(SMCFX_RVB, 0, 0, SMCFX_NFX)) != 0);
    CHECK(!strcmp(smcfx_select_caption(-1, 0, 1, SMCFX_NFX), "--"));
    CHECK(!strcmp(smcfx_select_caption(SMCFX_N, 1, 1, SMCFX_NFX), "--"));
    puts("PASS SMC FX captions: per-part LPF, main-bus tap navigation, HOME cycle, invalid buses");
    printf("Checks: %u\n", checks);
    return 0;
}
