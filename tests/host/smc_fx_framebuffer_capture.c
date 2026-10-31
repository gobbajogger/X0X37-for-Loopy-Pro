/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual firmware framebuffer capture for the existing SMC mixer/FX UI.
 * This includes the production host, canvas, app and SMC renderer. Hardware
 * transport is stubbed only as in smc_ui_test.c; screen drawing is not.
 * Use SMC_UI_TEST_FILE=tests/host/smc_fx_framebuffer_capture.c
 * with tests/host/smc_ui_test.sh, and set SMC_UI_OUT to an output folder. */
#define SMC_UI_MAIN smc_ui_regressions
#include "smc_ui_test.c"
#undef SMC_UI_MAIN

int main(void)
{
    setup(V_HOME, PART_303A);
    pref_t f = smc_target(0);
    pref_set(f, 64);
    knob(0, 1);
    smc_ui_begin(0, 1);
    ui_frame();
    shot("mixer-fx-home");

    setup(V_FX, PART_303A);
    knob(6, 1);
    smc_ui_begin(6, 1);
    ui_frame();
    shot("mixer-fx-page");
    return 0;
}
