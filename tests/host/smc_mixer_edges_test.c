/* SPDX-License-Identifier: GPL-3.0-only */
/* Self-contained host regressions for SMC pattern and UI gesture edges. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "app/smc_mixer_edges.h"

static void pattern(void)
{
    assert(smc_pattern_delta(4, -1, 1, 32) == 5);
    assert(smc_pattern_delta(4, 5, 1, 32) == 6); /* repeat accumulates */
    assert(smc_pattern_delta(4, 5, -1, 32) == -1); /* opposite cancels */
    assert(smc_pattern_delta(4, 6, -1, 32) == 5); /* opposite steps back */
    assert(smc_pattern_delta(4, -1, -1, 32) == 3);
    assert(smc_pattern_delta(0, -1, -1, 32) == -2);
    assert(smc_pattern_delta(31, -1, 1, 32) == -2);
    assert(smc_pattern_delta(31, 30, 1, 32) == -1);
    assert(smc_pattern_delta(31, 30, -1, 32) == 29);
    assert(smc_pattern_delta(3, -1, 0, 32) == -1);
}
static void presentation(void)
{
    assert(smc_ui_edge_alive(100, 1200, 0));
    assert(!smc_ui_edge_alive(1200, 1200, 0));
    assert(smc_ui_edge_alive(1200, 1200, 1));
    assert(smc_ui_edge_alive(5000, 1200, 1));
    assert(!smc_ui_edge_alive(5000, 1200, 0));
    assert(smc_ui_edge_alive(UINT32_MAX - 6u, 500u, 0));
    assert(smc_ui_edge_alive(499u, 500u, 0));
    assert(!smc_ui_edge_alive(500u, 500u, 0));
    assert(smc_ui_edge_alive(500u, 500u, 1));
}
int main(void)
{
    pattern(); presentation();
    puts("smc_mixer_edges: PASS (19 edge assertions)");
    return 0;
}
