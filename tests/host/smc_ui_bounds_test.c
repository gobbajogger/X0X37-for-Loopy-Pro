/* SPDX-License-Identifier: GPL-3.0-only */
/* Render real SMC strip controls under UBSan. The physical rows are M/S/R;
 * SELECT remains the separate title highlight, not a fourth button row. */
#define SMC_UI_MAIN smc_ui_baseline_main
#include "smc_ui_test.c"

int main(void)
{
    const int views[] = {V_HOME, V_SEQ, V_PART, V_MIX, V_FX, V_GLO, V_SONG};
    for (unsigned v = 0; v < sizeof views / sizeof *views; v++)
        for (int part = 0; part < NPARTS; part++) {
            setup(views[v], part);
            for (int locked = 0; locked < 2; locked++) {
                if (locked) CHECK(smc_lock_toggle());
                fader(0, locked); CHECK(smc_ui_active());
                pref_t target = smc_target(0);
                int value = pref_get(target), rec = ui.rec;
                screen_state_t before = screen_state();
                smc_ui_draw();
                unchanged(before);
                CHECK(smc_same(target, smc_target(0)));
                CHECK(pref_get(target) == value && ui.rec == rec);
            }
        }
    printf("smc_ui_bounds: PASS (%u checks; three physical rows; no mapping or UI redesign)\n", checks);
    return 0;
}
