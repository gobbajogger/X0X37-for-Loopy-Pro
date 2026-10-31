/* SPDX-License-Identifier: GPL-3.0-only */
/* Production queues, musical targets and popup over keyboard clip stages.
 * SCALE/CHORD/ARP belong to the keyboard; SMC keeps its own mixer context. */
#define SMC_UI_MAIN smc_clip_legacy_main
#include "smc_ui_test.c"
#undef SMC_UI_MAIN

static unsigned route;
static const char *const routes[] = {"BLE DAW", "BLE USER", "host USER"};

static void clip_turn(unsigned strip, int delta)
{
    uint8_t msg[3] = {route == 2 ? 0xBFu : 0xB0u,
        (uint8_t)(16u + strip), (uint8_t)(delta > 0 ? delta : 64 - delta)};
    if (route == 2)
        CHECK(smc_midi_host(0x0Bu | (uint32_t)msg[0] << 8 |
            (uint32_t)msg[1] << 16 | (uint32_t)msg[2] << 24));
    else smc_ble_midi(msg, sizeof msg);
    smc_poll();
    CHECK(smc_qr == smc_qw && smc_host_r == smc_host_w);
}

static void exercise_cover(uint8_t overlay, uint8_t help)
{
    uint8_t before_fields[CK_NE];
    pref_t target[8];
    int values[8];
    /* Capture actual independent SMC destinations before the confirmation
     * makes destination lookup deliberately return NONE. */
    ui.overlay = O_NONE; ui.help = 0;
    smc_ui_cancel();
    for (unsigned n = 0; n < 8; n++) {
        target[n] = smc_knob_target((int)n);
        CHECK(target[n].kind == R_ENG);
        const x0x_param_t *desc = pref_desc(target[n]);
        CHECK(desc && desc->max > 2);
        pref_set(target[n], desc->max / 2);
        values[n] = pref_get(target[n]);
    }
    ui.overlay = overlay; ui.help = help;
    screen_state_t keyboard = screen_state();
    memcpy(before_fields, fields, sizeof before_fields);
    for (unsigned n = 0; n < 8; n++) {
        clip_turn(n, 1);
        CHECK(pref_get(target[n]) == values[n] + (overlay != O_ASK));
        unchanged(keyboard);
        CHECK(!memcmp(before_fields, fields, sizeof before_fields));
        if (overlay == O_ASK || help) CHECK(!smc_ui_active());
        else {
            CHECK(smc_ui_active());
            CHECK(smc_ui.kind == 1 && smc_ui.strip == n && smc_ui.peer == 0);
            ui_frame(); /* Exercise the existing production renderer too. */
            unchanged(keyboard);
        }
    }
    ui.overlay = O_NONE; ui.help = 0;
    smc_ui_cancel();
    smc_poll();
    for (unsigned n = 0; n < 8; n++)
        CHECK(pref_get(target[n]) == values[n] + (overlay != O_ASK));
    CHECK(!memcmp(before_fields, fields, sizeof before_fields));
    /* Closing a cover does not replay a drained detent. A new detent still
     * reaches the same SMC destination and shows its dedicated popup. */
    int value = pref_get(target[0]);
    keyboard = screen_state();
    clip_turn(0, -1);
    CHECK(pref_get(target[0]) == value - 1);
    CHECK(smc_ui_active() && smc_ui.kind == 1 && smc_ui.strip == 0);
    CHECK(!memcmp(before_fields, fields, sizeof before_fields));
    unchanged(keyboard);
}

int main(void)
{
    for (route = 0; route < 3; route++) {
        setup(V_PART, PART_303A);
        proj.set.bt = PROJ_BT_SMC_CENTRAL | (route ? PROJ_BT_SMC_USER : 0);
        smc_view(V_HOME, 0); /* Dedicated SMC mixer, with a different keyboard page. */
        ui.mixsel = PART_808; ui.sel[0] = 10; ui.sel[1] = 8;
        *page_of(V_FX) = 1;
        for (unsigned mode = 0; mode < 5; mode++) {
            clip_grid_mode = mode < 4;
            ck.pg = (uint8_t)(mode < 4 ? mode : 1);
            memset(fields, 0, sizeof fields);
            exercise_cover(O_NONE, 0);
            exercise_cover(O_LIST, 0);
            exercise_cover(O_NONE, 1);
            exercise_cover(O_ASK, 0);
            printf("PASS %s keyboard clip mode %u: independent SMC edits/popup, "
                "keyboard fields/navigation preserved, modal drain\n", routes[route], mode);
        }
    }
    printf("smc_clip_visibility: PASS (%u checks)\n", checks);
    return 0;
}
