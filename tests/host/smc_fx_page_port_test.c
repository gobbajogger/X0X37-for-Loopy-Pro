/* SPDX-License-Identifier: GPL-3.0-only */
/* Mixer FX port: physical SELECT captions, peer-held screen ownership and
 * wrapping inactivity timeout. Uses production pure helpers, not DSP stubs. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app/smc_fx_caption.h"
#include "app/smc_mixer_edges.h"

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); \
    return 1; \
} } while (0)

int main(void)
{
    uint8_t held[2][16] = {{0}};
    const char *const main_caption[3] = {"RVB", "DLY", "MIX"};

    for (int bus = 0; bus < SMCFX_N; bus++) {
        const char *tap = bus < SMCFX_RVB ? "LPF" : main_caption[bus - SMCFX_RVB];
        CHECK(smcfx_bus(SMCFX_NOTE[bus]) == bus);
        CHECK(!strcmp(smcfx_select_caption(bus, 0, 0, SMCFX_REV), tap));
        CHECK(!strcmp(smcfx_select_caption(bus, 0, 1, SMCFX_REV), "LPF"));
        CHECK(!strcmp(smcfx_select_caption(bus, 1, 0, SMCFX_REV), "CYC"));
        CHECK(strlen(tap) <= 3u);
    }
    CHECK(!strcmp(smcfx_select_caption(-1, 1, 1, SMCFX_REV), "--"));
    CHECK(!strcmp(smcfx_select_caption(SMCFX_N, 0, 0, SMCFX_REV), "--"));

    /* The originating peer retains its held FX button while the other peer
     * can freely navigate and release its own press without stealing it. */
    CHECK(!smc_hold_any(held[0]) && !smc_hold_any(held[1]));
    CHECK(smc_hold_press(held[0], SMCFX_NOTE[SMCFX_909]));
    CHECK(!smc_hold_press(held[0], SMCFX_NOTE[SMCFX_909]));
    CHECK(smc_hold_any(held[0]) && !smc_hold_any(held[1]));
    CHECK(smc_hold_press(held[1], SMCFX_NOTE[SMCFX_FLT]));
    CHECK(smc_hold_any(held[0]) && smc_hold_any(held[1]));
    CHECK(smc_hold_release(held[1], SMCFX_NOTE[SMCFX_FLT]));
    CHECK(smc_hold_any(held[0]) && !smc_hold_any(held[1]));
    CHECK(smc_ui_edge_alive(1200u, 1200u, smc_hold_any(held[0])));
    CHECK(!smc_ui_edge_alive(1200u, 1200u, smc_hold_any(held[1])));
    CHECK(smc_hold_release(held[0], SMCFX_NOTE[SMCFX_909]));
    CHECK(!smc_hold_any(held[0]));
    CHECK(!smc_hold_release(held[0], SMCFX_NOTE[SMCFX_909]));

    /* Countdown stays correct while plat_ms wraps to zero. */
    CHECK(smc_ui_edge_alive(UINT32_MAX - 8u, 13u, 0));
    CHECK(smc_ui_edge_alive(12u, 13u, 0));
    CHECK(!smc_ui_edge_alive(13u, 13u, 0));
    CHECK(!smc_ui_edge_alive(14u, 13u, 0));
    CHECK(smc_ui_edge_alive(14u, 13u, 1));

    /* Invalid note and hold-tracking rules remain separate from FX mapping. */
    CHECK(smcfx_bus(0u) == -1);
    CHECK(smc_hold_note(6u)); /* HOME */
    CHECK(smc_hold_note(0x5Du)); /* STOP */
    CHECK(!smc_hold_note(40u)); /* CC fader */
    CHECK(smc_tracked_release(0x80u, 24u, 0u));
    CHECK(smc_tracked_release(0x90u, 24u, 0u));
    CHECK(!smc_tracked_release(0x90u, 24u, 127u));
    printf("PASS mixer FX port: %u peer-hold, page-caption and wrap checks\n", checks);
    return 0;
}
