/* SPDX-License-Identifier: GPL-3.0-only */
/* Production local controls and autosave, using the existing clip/USB fixture. */
#define main clip_fixture_main
#include "smk_clip_knob_test.c"
#undef main

static void clean_saved_state(void)
{
    ui.dirty = ui.saved_t = save_calls = 0;
    ui.overlay = O_NONE;
    seq.playing = seq.req_start = seq.req_cont = seq.req_stop = ui.rec = 0;
    fm1_in.keys = 0;
}

static void arrow_right(void)
{
    fm1_in.buttons |= 1u << (SMK_BTN_LEFT + 1u); run(1);
    fm1_in.buttons &= ~(1u << (SMK_BTN_LEFT + 1u)); run(1);
}

int main(void)
{
    for (unsigned i = 0; i < NE; i++) { panel.enc[i] = i; panel.dir[i] = 1; }
    for (unsigned i = 0; i < NB; i++) panel.btn[i] = 4u + i;
    proj.set.ck_ch = 2; seq.bpm = 120; clip_grid_mode = 1;
    run(100); tap(B_SEL);
    fm1_in.enc_steps[0] = 1; run(20);
    check("local knob selects ROOT", (ck.pg >> 4) == CK_E_ROOT);
    clean_saved_state();
    fm1_in.enc_steps[1] = 1; run(20);
    check("local ROOT knob edit marks the clean project dirty", proj.set.ck_root == 1 && ui.dirty == 1);
    run(AUTOSAVE_QUIET + 1); autosave();
    check("local ROOT knob edit reaches autosave", save_calls == 1 && ui.dirty == 0);
    clean_saved_state(); arrow_right();
    check("local ROOT arrow edit marks the clean project dirty", proj.set.ck_root == 2 && ui.dirty == 1);
    ui.overlay = O_ASK;
    run(AUTOSAVE_QUIET + 1); autosave();
    check("unsaved edit remains dirty during confirmation", save_calls == 0 && ui.dirty == 1);
    ui.overlay = O_NONE; autosave();
    check("local ROOT arrow edit reaches autosave", save_calls == 1 && ui.dirty == 0);

    for (unsigned f = 0; f < CK_E_LATCH; f++) {
        clean_saved_state();
        ck.pg = (uint8_t)(1u | f << 4);
        unsigned old = ck_get(f), next = old ? 0u : 1u;
        ck_set(f, next);
        check("persistent SCALE/CHORD/ARP field marks dirty once", ck_get(f) == next && ui.dirty == 1);
        ck_set(f, next);
        check("unchanged persistent field adds no dirty mark", ui.dirty == 1);
        run(AUTOSAVE_QUIET + 1); autosave();
        check("persistent field reaches autosave", save_calls == 1 && ui.dirty == 0);
    }
    clean_saved_state(); ck.pg = (uint8_t)(1u | CK_E_ON << 4);
    ck_set(CK_E_ON, 1); ck_set(CK_E_ON, 0);
    ck_set(CK_E_LATCH, CK_F_LATCH); ck_set(CK_E_LATCH, 0);
    check("runtime ON/LATCH changes do not dirty the saved project", ui.dirty == 0);
    ck.pg = (uint8_t)(1u | CK_E_ON << 4);
    smk_ck_smc_nav(0x63);
    check("SMC runtime ON navigation does not dirty the project", ui.dirty == 0);
    ck.pg = (uint8_t)(1u | CK_E_ROOT << 4);
    smk_ck_smc_turn(1, 1);
    check("SMC persistent edit uses one shared dirty mark", ui.dirty == 1);
    printf("clip settings persistence: %s\n", fails ? "FAIL" : "PASS");
    return fails != 0;
}
