/* SPDX-License-Identifier: GPL-3.0-only */
/* Pure, production-header host regressions. No private engine inputs. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app/smc_mixer_edges.h"

static unsigned checks;
#define CHECK(expr) do { checks++; if (!(expr)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; \
} } while (0)

static int pattern(void)
{
    for (int p = 0; p < 32; p++) {
        CHECK(smc_pattern_delta(p, -1, 1, 32) == (p == 31 ? -2 : p + 1));
        CHECK(smc_pattern_delta(p, -1, -1, 32) == (p == 0 ? -2 : p - 1));
        CHECK(smc_pattern_delta(p, p, 0, 32) == -1);
    }
    CHECK(smc_pattern_delta(4, 5, 1, 32) == 6);
    CHECK(smc_pattern_delta(4, 6, -1, 32) == 5);
    CHECK(smc_pattern_delta(4, 5, -1, 32) == -1);
    CHECK(smc_pattern_delta(31, 30, 1, 32) == -1);
    CHECK(smc_pattern_delta(0, 1, -1, 32) == -1);
    return 0;
}

static int physical(void)
{
    uint8_t held[16] = {0};
    for (unsigned n = 0; n < 128; n++) {
        int tracked = n < 32u || (n >= 0x2eu && n <= 0x31u) ||
                      (n >= 0x5bu && n <= 0x63u);
        CHECK(smc_hold_note(n) == tracked);
        CHECK(smc_tracked_release(0x80u,n,0) == tracked);
        CHECK(smc_tracked_release(0x90u,n,0) == tracked);
        CHECK(!smc_tracked_release(0x90u,n,127));
        CHECK(!smc_tracked_release(0xb0u,n,0));
        CHECK(!smc_tracked_release(0x80u,n,128));
        CHECK(smc_hold_press(held,n));
        if (tracked) {
            CHECK(!smc_hold_press(held,n));
            CHECK(smc_hold_any(held));
            CHECK(smc_hold_release(held,n));
            CHECK(!smc_hold_release(held,n));
        } else {
            CHECK(!smc_hold_release(held,n));
        }
    }
    CHECK(!smc_hold_any(held));
    CHECK(smc_hold_press(held,8));
    CHECK(smc_hold_press(held,0x5e));
    CHECK(smc_hold_release(held,8));
    CHECK(smc_hold_any(held));
    CHECK(smc_hold_release(held,0x5e));
    CHECK(!smc_hold_any(held));
    /* Two SMCs' held buttons remain separately owned. */
    uint8_t peer1[16] = {0}, peer2[16] = {0};
    CHECK(smc_hold_press(peer1,16));
    CHECK(smc_hold_press(peer2,16));
    CHECK(smc_hold_release(peer1,16));
    CHECK(smc_hold_any(peer2));
    CHECK(smc_hold_release(peer2,16));
    CHECK(!smc_hold_any(peer2));
    return 0;
}

static int deadline(void)
{
    CHECK(smc_ui_edge_alive(100,1200,0));
    CHECK(!smc_ui_edge_alive(1200,1200,0));
    CHECK(smc_ui_edge_alive(5000,1200,1));
    CHECK(!smc_ui_edge_alive(5000,1200,0));
    CHECK(smc_ui_edge_alive(UINT32_MAX-6,500,0));
    CHECK(smc_ui_edge_alive(499,500,0));
    CHECK(!smc_ui_edge_alive(500,500,0));
    CHECK(smc_ui_edge_alive(500,500,1));
    return 0;
}

int main(void)
{
    if (pattern() || physical() || deadline()) return 1;
    printf("PASS SMC mixer edges (%u assertions)\n",checks);
    return 0;
}
