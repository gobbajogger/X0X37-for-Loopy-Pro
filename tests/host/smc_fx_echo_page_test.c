/* SPDX-License-Identifier: GPL-3.0-only */
/* SMC FX third SELECT page: ROOM / TIGHT / ECHO. Port of 7b2d1726 (#688).
 * Drum suits stay the shared 909/808/BREAK set from 470c94c. */
#include <stdio.h>
#include <string.h>
#include "app/smc_fx_map.h"
#include "app/smc_fx_strip.h"

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); \
    return 1; \
} } while (0)

int main(void)
{
    static const uint8_t echo[SMCFX_N] = {
        SMCFX_DLY, SMCFX_REV, SMCFX_REV, SMCFX_REV,
        SMCFX_DLY, SMCFX_STUT, SMCFX_SHUF, SMCFX_GATE8
    };
    static const uint8_t drum_suit[] = {
        SMCFX_REV, SMCFX_DLY, SMCFX_STUT, SMCFX_SHUF, SMCFX_GATE, SMCFX_HP
    };
    const int drum_buses[] = { SMCFX_909, SMCFX_808, SMCFX_BRK };
    int i, bus, page, b;
    smcfx_strip_t s;

    CHECK(SMCFX_NPAGE == 3);
    CHECK(!strcmp(SMCFX_PAGE_NAME[0], "ROOM"));
    CHECK(!strcmp(SMCFX_PAGE_NAME[1], "TIGHT"));
    CHECK(!strcmp(SMCFX_PAGE_NAME[2], "ECHO"));

    for (bus = 0; bus < SMCFX_N; bus++) {
        CHECK(SMCFX_PAGE[2][bus] == echo[bus]);
        for (page = 0; page < SMCFX_NPAGE; page++)
            CHECK(smcfx_suit_ok(bus, SMCFX_PAGE[page][bus]));
    }

    for (i = 0; i < 3; i++) {
        bus = drum_buses[i];
        CHECK(SMCFX_SUIT_N[bus] == 6);
        for (b = 0; b < 6; b++)
            CHECK(SMCFX_SUIT[bus][b] == drum_suit[b]);
    }

    smcfx_strip_init(&s);
    CHECK(SMCFX_STRIP_PAGES == 3);
    for (bus = 0; bus < SMCFX_N; bus++) {
        CHECK(smcfx_strip_get(&s, 2, bus, SMCFX_REC) == SMCFX_EMPTY);
        CHECK(smcfx_strip_get(&s, 2, bus, SMCFX_SOLO) == SMCFX_EMPTY);
        CHECK(smcfx_strip_get(&s, 2, bus, SMCFX_MUTE) == SMCFX_EMPTY);
        CHECK(smcfx_strip_get(&s, 2, bus, SMCFX_SELECT) == echo[bus]);
        CHECK(smcfx_strip_press(&s, (unsigned)(24 + bus), bus, 2));
        CHECK(smcfx_strip_key_page(&s, (unsigned)(24 + bus)) == 2);
        CHECK(smcfx_strip_live_fx(&s, bus) == echo[bus]);
        CHECK(smcfx_strip_release(&s, (unsigned)(24 + bus)) == bus);
    }

    printf("PASS SMC FX ECHO page: %u assertions\n", checks);
    return 0;
}
