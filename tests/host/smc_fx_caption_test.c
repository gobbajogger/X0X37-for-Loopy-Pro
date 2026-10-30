/* SPDX-License-Identifier: GPL-3.0-only */
/* Real integrated SMC FX bus map and truthful tap/hold captions. */
#include <stdio.h>
#include <string.h>
#include "app/smc_fx_caption.h"

static unsigned checks;
#define CHECK(expr) do { checks++; if (!(expr)) { \
    fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#expr);return 1; \
} } while (0)

int main(void)
{
    static const char *const main_tap[3] = {"RVB", "DLY", "MIX"};
    for (int bus = 0; bus < SMCFX_N; bus++) {
        CHECK(smcfx_bus(SMCFX_NOTE[bus]) == bus);
        CHECK(smcfx_is_main(bus) == (bus >= SMCFX_RVB));
        for (int page = 0; page < SMCFX_NPAGE; page++) {
            unsigned fx = SMCFX_PAGE[page][bus];
            const char *tap = smcfx_select_caption(bus, 0, 0, fx);
            const char *hold = smcfx_select_caption(bus, 0, 1, fx);
            CHECK(smcfx_suit_ok(bus, fx));
            CHECK(!strcmp(hold, SMCFX_SHORT[fx]));
            CHECK(!strcmp(smcfx_select_caption(bus, 1, 0, fx), "CYC"));
            CHECK(bus < SMCFX_RVB ? !strcmp(tap,SMCFX_SHORT[fx]) :
                  !strcmp(tap, main_tap[bus - SMCFX_RVB]));
            CHECK(strlen(tap) <= 4u);
        }
        CHECK(!strcmp(smcfx_select_caption(bus, 0, 1, SMCFX_NFX), "SEL"));
    }
    CHECK(!strcmp(smcfx_select_caption(-1, 0, 0, SMCFX_REV),"--"));
    CHECK(!strcmp(smcfx_select_caption(SMCFX_N, 0, 0, SMCFX_REV),"--"));
    /* ROOM preset advertises REV on the RVB strip; a tap navigates rather
     * than throwing that effect. Hold feedback must still read REV. */
    CHECK(!strcmp(smcfx_select_caption(SMCFX_RVB,0,0,SMCFX_REV),"RVB"));
    CHECK(!strcmp(smcfx_select_caption(SMCFX_RVB,0,1,SMCFX_REV),"REV"));
    printf("PASS integrated SMC FX captions (%u assertions)\n",checks);
    return 0;
}
