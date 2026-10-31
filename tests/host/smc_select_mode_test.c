/* SPDX-License-Identifier: GPL-3.0-only */
/* Real SMC MIDI queue/dispatch + UI/engine: SELECT belongs to its press mode. */
#include <stdint.h>
#define SMC_UI_SEND_IMPLEMENTED 1
static uint8_t select_led_velocity[8];
int plat_ble_smc_send(const uint8_t *m, uint8_t n)
{
    if (n == 3 && m[0] == 0x90 && m[1] >= 24 && m[1] < 32)
        select_led_velocity[m[1] - 24] = m[2];
    return 1;
}
#define SMC_UI_MAIN smc_ui_existing_main
#include "smc_ui_test.c"
#undef SMC_UI_MAIN

static void check_select_led(int strip, int on)
{
    smc_frame();
    CHECK(smc_led(24 + strip) == !!on);
    CHECK(select_led_velocity[strip] == (on ? 127 : 0));
}
static void check_main_navigation_leds(void)
{
    check_select_led(5, smc.view == V_FX && smc_page_now() == 0);
    check_select_led(6, smc.view == V_FX && smc_page_now() == 1);
    check_select_led(7, smc.view == V_MIX && smc_page_now() == 3);
}
static void check_plain_leds(void)
{
    const int views[] = {V_HOME, V_PART, V_SEQ, V_GEN, V_MIX, V_GLO, V_SONG};
    for (unsigned v = 0; v < sizeof views / sizeof *views; v++) for (int part = 0; part < NPARTS; part++) {
        if (views[v] == V_GEN && part != PART_303A && part != PART_303B) continue;
        setup(views[v], part);
        if (views[v] == V_SEQ && part < NKIT) {
            /* Drum SEQ retains sound-bank selection, including the final partial bank. */
            for (int track = 0; track < NDRUM; track++) {
                message(0x90, track < 8 ? 0x2e : 0x2f, 127);
                message(0x80, track < 8 ? 0x2e : 0x2f, 0);
                message(0x90, 24 + track % 8, 127);
                message(0x80, 24 + track % 8, 0);
                CHECK(ui.sel[part] == 0); /* keyboard selection is independent */
                for (int strip = 0; strip < 8; strip++) check_select_led(strip, strip == track % 8);
            }
        } else {
            for (int strip = 0; strip < NPARTS; strip++) check_select_led(strip, strip == (part==2?4:part==4?2:part));
        }
    }
}
static void check_plain_select(void)
{
    const int views[] = {V_HOME, V_PART, V_SEQ, V_GEN, V_MIX, V_GLO, V_SONG};
    for (unsigned v = 0; v < sizeof views / sizeof *views; v++) for (int strip = 0; strip < 8; strip++) {
        setup(views[v], strip == PART_BRK ? PART_303A : PART_BRK);
        if (views[v] == V_GEN) { smc.part = PART_303A; build_pages(); }
        screen_state_t keyboard = screen_state();
        int rec = ui.rec;
        message(0x90, 24 + strip, 127);
        CHECK(!smc.fx_down && !smc.fx_main_wait);
        if (strip < NPARTS) { CHECK(smc.part == (strip==2?4:strip==4?2:strip)); CHECK(smc.view == V_PART); check_select_led(strip, 1); }
        else { CHECK(smc.view == (strip == 7 ? V_MIX : V_FX)); /* SELECT 7 opens the mixer on page 0; on the mixer it steps to the next page. */
               CHECK(smc_page_now() == (strip == 5 ? 0 : strip == 6 ? 1 : 3)); }
        int view = smc.view, page = smc_page_now(), part = smc.part;
        message(0x90, 24 + strip, 127); /* repeat cannot acquire FX after navigation */
        now_ms += 200; smc_poll();
        CHECK(!smc.fx_down && !smc.fx_main_wait);
        message(0x80, 24 + strip, 0);
        CHECK(smc.view == view && smc_page_now() == page && smc.part == part);
        if (strip < NPARTS) check_select_led(strip, 1);
        else check_main_navigation_leds();
        CHECK(ui.rec == rec && !seq.req_start && !seq.req_stop);
        CHECK(ui.view == keyboard.view && ui.part == keyboard.part && ui.mixsel == keyboard.mixsel);
        CHECK(!memcmp(ui.page, keyboard.page, sizeof ui.page));
    }
}
static void check_fx_and_release(void)
{
    for (int strip = 0; strip < 8; strip++) {
        setup(V_FX, PART_303A);
        int part = smc.part;
        message(0x90, 24 + strip, 127);
        if (strip < 5) { CHECK(smc.fx_down == (1u << (strip==2?4:strip==4?2:strip)));
            for (int lane = 0; lane < NPARTS; lane++) check_select_led(lane, lane == strip);
        }
        else { CHECK(smc.fx_down == (1u << strip)); CHECK(smc.fx_main_wait == (1u << (strip - 5)));
            check_main_navigation_leds(); /* owner: quick taps also play the assigned effect immediately */
        }
        now_ms += 200; smc_poll(); CHECK(smc.fx_down == (1u << (strip==2?4:strip==4?2:strip)));
        if (strip < NPARTS) check_select_led(strip, 1);
        else check_main_navigation_leds();
        CHECK(smc.view == V_FX && smc.part == part);
        message(0x90, 24 + strip, 0); /* captured preset sends NoteOn zero */
        CHECK(!smc.fx_down && !smc.fx_main_wait);
        CHECK(smc.view == V_FX && smc.part == part);
        if (strip < NPARTS) check_select_led(strip, 0);
        else check_main_navigation_leds();
    }
    setup(V_FX, PART_303A);
    message(0x90, 27, 127);
    CHECK(smc.fx_effect[SMCFX_303B] == SMCFX_LP); /* ROOM defaults: 303B SELECT is low-pass */
}
static void check_main_taps(void)
{
    for (int strip = 5; strip < 8; strip++) {
        setup(V_FX, PART_303A);
        message(0x90, 24 + strip, 127);
        message(0x80, 24 + strip, 0);
        CHECK(smc.view == (strip == 7 ? V_MIX : V_FX));
        CHECK(smc_page_now() == (strip == 5 ? 0 : strip == 6 ? 1 : 3));
        CHECK(!smc.fx_down && !smc.fx_main_wait);
        check_select_led(5, strip == 5); check_select_led(6, strip == 6); check_select_led(7, strip == 7);
    }
    setup(V_FX, PART_303A);
    message(0x90, 27, 127); CHECK(smc.fx_down);
    message(0x90, 31, 127); message(0x80, 31, 0);
    CHECK(smc.view == V_MIX && !smc.fx_down && !smc.fx_main_wait);
    message(0x80, 27, 0); CHECK(smc.view == V_MIX && !smc.fx_down);
}
static void check_mode_changes(void)
{
    const int views[] = {V_HOME, V_PART, V_SEQ, V_GEN, V_MIX, V_GLO, V_SONG};
    for (unsigned v = 0; v < sizeof views / sizeof *views; v++) for (int strip = 0; strip < 8; strip++) for (int hold = 0; hold < 2; hold++) {
        setup(V_FX, PART_303A);
        message(0x90, 24 + strip, 127);
        if (hold) { now_ms += 200; smc_poll(); }
        smc.part = views[v] == V_SEQ || views[v] == V_MIX ? PART_909 : PART_303A;
        smc_view(views[v], -1);
        int part = smc.part, page = smc_page_now();
        smc_poll(); CHECK(!smc.fx_down && !smc.fx_main_wait);
        check_main_navigation_leds();
        if (views[v] != V_SEQ && views[v] != V_MIX)
            for (int lane = 0; lane < NPARTS; lane++) check_select_led(lane, lane == (part==2?4:part==4?2:part));
        message(0x80, 24 + strip, 0);
        CHECK(!smc.fx_down && !smc.fx_main_wait);
        CHECK(smc.view == views[v] && smc.part == part && smc_page_now() == page);
        /* A late release after returning to FX must not navigate a main strip. */
        smc_view(V_FX, strip == 6 ? 0 : 1);
        int fx_page = smc_page_now();
        message(0x80, 24 + strip, 0);
        CHECK(smc.view == V_FX && smc_page_now() == fx_page && !smc.fx_down);
        for (int lane = 0; lane < NPARTS; lane++) check_select_led(lane, 0);
        check_main_navigation_leds();
    }
    /* Release clears its original FX before a poll can reconcile a new drum bank. */
    setup(V_FX, PART_303A); message(0x90, 27, 127); CHECK(smc.fx_down);
    smc.part = PART_909; smc_view(V_SEQ, 0); smc_release(27);
    CHECK(!smc.fx_down && !smc.fx_main_wait);
}
int main(void)
{
    check_plain_leds(); check_plain_select();
    puts("PASS SELECT LEDs and MIDI feedback: independent SMC selection, keyboard navigation unchanged");
    check_fx_and_release(); check_main_taps(); check_mode_changes();
    printf("PASS SMC SELECT mode ownership: %u production-input checks\n", checks);
    return 0;
}
