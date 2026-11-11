/* Owner audit contracts use the production keyboard and SMC together. */
#define SMC_UI_MAIN legacy_ui_main
#include "smc_ui_test.c"
#undef SMC_UI_MAIN

static void tap_smc(int note)
{
    message(0x90, note, 127);
    message(0x80, note, 0);
}

static void held_keyboard_steps_survive_select(void)
{
    setup(V_SEQ, PART_303A);
    bass_step_key(0, 1);
    CHECK(ui.held_step == 0);
    uint8_t owner = ui.held_step_part, pattern = ui.held_step_pat;
    int keyboard_page = cur_page();
    tap_smc(24); /* Select 909 using the actual remote input path. */
    CHECK(smc.part == PART_909 && smc.view == V_PART);
    CHECK(ui.held_step == 0 && ui.held_step_part == owner && ui.held_step_pat == pattern);
    CHECK(ui.part == PART_303A && ui.view == V_SEQ && cur_page() == keyboard_page);
    bass_step_key(0, 0);
    CHECK(ui.held_step == -1);

    setup(V_SEQ, PART_909);
    ui.sel[PART_909] = 3;
    drum_step(PART_909, 2, 1);
    CHECK(ui.hold_step == 2);
    owner = ui.hold_drum; pattern = ui.hold_pat;
    keyboard_page = cur_page();
    smc_view(V_HOME, 0);
    tap_smc(28); /* Select Chicago; the held drum remains keyboard-owned. */
    CHECK(smc.part == PART_303A && smc.view == V_PART);
    CHECK(ui.hold_step == 2 && ui.hold_drum == owner && ui.hold_pat == pattern);
    CHECK(ui.part == PART_909 && ui.sel[PART_909] == 3 && ui.view == V_SEQ && cur_page() == keyboard_page);
    drum_step(PART_909, 2, 0);
    CHECK(ui.hold_step == -1);
}

static void song_navigation_preserves_motion(void)
{
    setup(V_SONG, PART_303A);
    lane_t before[sizeof proj.arr.lane / sizeof proj.arr.lane[0]];
    memcpy(before, proj.arr.lane, sizeof before);
    screen_state_t keyboard = screen_state();
    smc_view(V_SONG, 0);
    tap_smc(0x60); tap_smc(0x61); tap_smc(0x62); tap_smc(0x63);
    knob(0, 1); knob(0, -1);
    CHECK(!memcmp(before, proj.arr.lane, sizeof before));
    unchanged(keyboard);
}

int main(void)
{
    held_keyboard_steps_survive_select();
    song_navigation_preserves_motion();
    printf("smc_audit_contract: PASS (%u checks; held 303/drum steps and song lanes)\n", checks);
    return 0;
}
