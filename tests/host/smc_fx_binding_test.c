/* SPDX-License-Identifier: GPL-3.0-only */
/* Replay every physical DAW strip button through real dispatch, not captions alone. */
#include <stdint.h>
static uint8_t fx_led[128];
int plat_ble_smc_send(const uint8_t *m, uint8_t n)
{
    if (n == 3 && m[0] == 0x90) fx_led[m[1]] = m[2];
    return 1;
}
#define SMC_UI_SEND_IMPLEMENTED 1
#define SMC_UI_MAIN smc_ui_existing_main
#include "smc_ui_test.c"
#undef SMC_UI_MAIN

static void check_wire(int note)
{
    smc_frame(); CHECK(fx_led[note] == (smc_led(note) ? 127 : 0));
}
static void all_rows(void)
{
    const int notes[4][8] = {
        {0,1,2,3,4,5,6,7}, {8,9,10,11,12,13,14,15},
        {16,17,18,19,20,21,22,23}, {24,25,26,27,28,29,30,31}
    };
    for (int row = 0; row < 4; row++) for (int strip = 0; strip < 8; strip++) {
        setup(V_FX, PART_303A);
        int note = notes[row][strip];
        const char *caption = smc_button_caption(note);
        if (row == 3) CHECK(!strcmp(caption, SMCFX_SHORT[SMCFX_PAGE[0][strip]]));
        else if (strip < 5 || row == 2) CHECK(!strcmp(caption, row == 0 ? "R" : row == 1 ? "S" : "M"));
        int muted = strip < 5 ? part_muted(strip) : 0;
        pref_t tail = PR(R_ENG, T_FX, 0, strip == 5 ? FX_RV_LEVEL : strip == 6 ? FX_DL_LEVEL : FX_VOLUME);
        if (row == 2 && strip >= 5) pref_set(tail, 100);
        message(0x90, note, 127);
        if (row == 0) {
            if (strip < 5) { CHECK(ui.part == strip); CHECK(ui.rec); CHECK(ui.view == V_FX); }
            else if (strip == 5) CHECK(ui.view == V_FX && ui.overlay == O_LIST);
            else CHECK(ui.view == V_HOME);
        } else if (row == 1) {
            if (strip < 5) CHECK(smc.solo & (1u << strip));
            else CHECK(ui.view == (strip == 5 ? V_MIX : strip == 6 ? V_GLO : V_PART));
        } else if (row == 2) {
            if (strip < 5) CHECK(part_muted(strip) != muted);
            else CHECK(pref_get(tail) == 0);
        } else {
            if (strip < 5) CHECK(smc.fx_down == (1u << strip));
            else { CHECK(!smc.fx_down); CHECK(smc.fx_main_wait == (1u << (strip - 5))); }
            now_ms += 180; smc_poll();
            CHECK(smc.fx_down == (1u << strip)); CHECK(ui.view == V_FX && ui.part == PART_303A);
            CHECK(smc.fx_effect[strip] == SMCFX_PAGE[0][strip]);
            if (strip < 5) CHECK(smc_led(note));
            message(0x90, note, 127); CHECK(smc.fx_down == (1u << strip));
        }
        check_wire(note);
        int rec = ui.rec, part = ui.part, view = ui.view;
        message(0x80, note, 0);
        CHECK(ui.rec == rec && ui.part == part && ui.view == view);
        if (row == 1 && strip < 5) CHECK(!smc.solo);
        CHECK(!smc.fx_down && !smc.fx_main_wait); check_wire(note);
    }
    puts("PASS FX rows: all32 DAW actions, actual target/transport/UI changes, release, caption and wire feedback");
}
static void guards_and_main_tap(void)
{
    for (int strip = 0; strip < 8; strip++) {
        setup(V_FX, PART_303A); open_list_of_pages();
        message(0x90, 24 + strip, 127); CHECK(!smc.fx_down && !smc.fx_main_wait);
        message(0x80, 24 + strip, 0); CHECK(!smc.fx_down);
        setup(V_FX, PART_303A); clip_grid_mode = 1;
        message(0x90, 24 + strip, 127); CHECK(!smc.fx_down && !smc.fx_main_wait);
        message(0x80, 24 + strip, 0); CHECK(!smc.fx_down);
        setup(V_SEQ, PART_909); CHECK(smc_lock_toggle());
        set_view(V_FX); build_pages();
        message(0x90, 24 + strip, 127); CHECK(!smc.fx_down && !smc.fx_main_wait);
        CHECK(smc.locked && ui.view == V_FX); CHECK(!strcmp(smc_button_caption(24 + strip), "X"));
        message(0x80, 24 + strip, 0); CHECK(!smc.fx_down);
    }
    setup(V_FX, PART_303A);
    message(0x90,31,127); CHECK(ui.view == V_FX && !smc.fx_down);
    message(0x90,31,127); CHECK(ui.view == V_FX && !smc.fx_down);
    message(0x80,31,0); CHECK(ui.view == V_MIX && cur_page() == 3);
    CHECK(!smc.fx_owned && !smc.fx_main_wait && !smc.fx_down);
    setup(V_FX, PART_303A); message(0x90,24,127); CHECK(smc.fx_down);
    set_view(V_HOME); build_pages(); smc_poll(); CHECK(!smc.fx_down);
    message(0x80,24,0); CHECK(ui.view == V_HOME && !smc.fx_owned);
    puts("PASS FX ownership: overlays/clip/drum lock guarded, main tap release once, navigation clears owned holds");
}
static void decode(void)
{
    setup(V_FX, PART_303A);
    message(0x91,24,127); CHECK(!smc.fx_down);
    message(0xB0,24,127); CHECK(!smc.fx_down); /* no button CC mapping */
    CHECK(!smc_midi_host(9u | 0x90u << 8 | 24u << 16 | 127u << 24));
    proj.set.bt |= PROJ_BT_SMC_USER;
    CHECK(smc_midi_host(9u | 0x9Fu << 8 | 24u << 16 | 127u << 24)); smc_poll();
    CHECK(smc.fx_down == 1u);
    CHECK(smc_midi_host(8u | 0x8Fu << 8 | 24u << 16)); smc_poll(); CHECK(!smc.fx_down);
    puts("PASS FX decode: DAW channel1, opt-in hosted channel16, CC rejection and release routing");
}
int main(void)
{
    all_rows(); guards_and_main_tap(); decode();
    printf("PASS full SMC FX binding: %u checks\n", checks);
    return 0;
}
