/* SPDX-License-Identifier: GPL-3.0-only */
#define SMC_UI_MAIN smc_ui_fixture_main
#include "smc_ui_test.c"
static void panel_tap(int b)
{
    held_btn = 1u << b; now_ms++; input();
    held_btn = 0; now_ms++; input();
}
static void carried_step(int part, int bank)
{
    setup(V_SEQ, part);
    ui.spage = (uint8_t)bank;
    int step = bank * 16 + 2;
    if (part < NKIT) {
        proj.pat[0].drum[part].len = NSTEPS;
        sm_set(proj.pat[0].drum[part].hit[0], step);
    } else {
        proj.pat[0].bass[part-PART_303A].len = NSTEPS;
        proj.pat[0].bass[part-PART_303A].step[step].flags = G_NOTE;
        proj.pat[0].bass[part-PART_303A].step[step].note = 48;
    }
    held_keys = 1u << WHITE_KEY[2]; input();
    CHECK(part < NKIT ? ui.hold_step == step : ui.held_step == step);
    panel_tap(B_EDIT);
    CHECK(ui.view == V_PART);
    accel_off = 1;
    enc_acc[part < NKIT ? EN_K1 : EN_K2] = -1; input();
    CHECK(motion_count(proj.arr.lane) == 1);
    static lane_t kept[NLANE]; memcpy(kept, proj.arr.lane, sizeof kept);
    held_keys = 0; input();
    CHECK(part < NKIT ? ui.hold_step < 0 : ui.held_step < 0);
    CHECK(!memcmp(kept, proj.arr.lane, sizeof kept));
    panel_tap(B_SEQ);
    CHECK(ui.view == V_SEQ);
    held_keys = 1u << WHITE_KEY[2]; input();
    enc_acc[EN_K1] = 2; input();
    now_ms += STEP_TAP_MS; /* an actual hold; short unedited drum taps toggle */
    held_keys = 0; input();
    CHECK(!memcmp(kept, proj.arr.lane, sizeof kept));
    CHECK(motion_count(proj.arr.lane) == 1);
    if (part >= NKIT) CHECK(proj.pat[0].bass[part-PART_303A].step[step].note == 50);
    else CHECK(sm_get(proj.pat[0].drum[part].hit[0], step));
    accel_off = 0;
}
int main(void)
{
    for (int part = PART_909; part <= PART_303B; ++part)
        for (int bank = 0; bank < 4; ++bank) carried_step(part, bank);
    printf("held-step-release: PASS (%u checks, both kits/voices, all four step banks)\n", checks);
    return 0;
}
