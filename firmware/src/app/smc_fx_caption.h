/* SPDX-License-Identifier: GPL-3.0-only */
/* Describe the button action, not the previously assigned momentary FX.
 * A main-bus tap navigates; holding it briefly activates low-pass instead.
 * The existing HOME+SELECT gesture cycles the assignment without playing it. */
#pragma once
#include "smc_fx_map.h"

static inline const char *smcfx_select_caption(int bus, int home_held, int fx_active,
                                               unsigned assigned)
{
    (void)assigned; /* #714 passes the assignment; SELECT plays low-pass (#725), so show that. */
    if (bus < 0 || bus >= SMCFX_N)
        return "--";
    if (home_held)
        return "CYC";
    if (fx_active)
        return SMCFX_SHORT[SMCFX_LP];
    if (bus == SMCFX_RVB)
        return "RVB";
    if (bus == SMCFX_MDLY)
        return "DLY";
    if (bus == SMCFX_FLT)
        return "MIX";
    return SMCFX_SHORT[SMCFX_LP];
}
