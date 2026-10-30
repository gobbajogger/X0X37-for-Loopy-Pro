/* SPDX-License-Identifier: GPL-3.0-only */
/* FX SELECT labels follow the real mapped action on the integrated branch.
 * Main-bus taps navigate, but a held SELECT performs its assigned effect.
 * Do not replace the assigned FX or change audio just to adjust the label. */
#pragma once
#include "smc_fx_map.h"

static inline const char *smcfx_select_caption(int bus, int home_held,
                                               int active, unsigned assigned)
{
    if (bus < 0 || bus >= SMCFX_N) return "--";
    if (home_held) return "CYC";
    if (smcfx_is_main(bus) && !active)
        return bus == SMCFX_RVB ? "RVB" : bus == SMCFX_MDLY ? "DLY" : "MIX";
    return assigned < SMCFX_NFX ? SMCFX_SHORT[assigned] : "SEL";
}
