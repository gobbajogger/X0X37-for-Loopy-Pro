/* SPDX-License-Identifier: GPL-3.0-only */
/* Production SMC pages against real UI, engine and note queue. */
#define SMC_UI_MAIN smc_ui_test_main
#include "smc_ui_test.c"
#undef SMC_UI_MAIN

static void page_note(int status, int note, int value)
{
    uint8_t msg[3] = {(uint8_t)status, (uint8_t)note, (uint8_t)value};
    smc_ble_midi(msg, 3);
    smc_poll();
}
static void page_tap(int note)
{
    page_note(0x90, note, 127);
    page_note(0x80, note, 0);
}
int main(void)
{
    setup(V_MIX, PART_909);
    /* The very first mixer read never captures an already moved keyboard page. */
    *page_of(V_MIX) = 3; build_pages();
    CHECK(cur_page() == 3 && smc_page_now() == 0);
    smc_page_set(3);
    *page_of(V_MIX) = 0; build_pages();
    CHECK(smc_page_now() == 3 && smc_led(31)); /* MIX SELECT LED is mixer-owned */
    CHECK(cur_page() == 0);
    ui.sel[PART_909] = 0;
    ui.sel[PART_808] = 1;
    build_pages();
    smc_page_set(1);
    CHECK(smc_page_now() == 1);
    CHECK(smc_drum_mix() == PART_909);
    CHECK(smc_led(24)); /* SELECT LED owns the chosen 909 track */
    pref_t first = smc_target(0);
    *page_of(V_MIX) = 2;
    build_pages();
    CHECK(cur_page() == 2 && smc_page_now() == 1);
    CHECK(smc_drum_mix() == PART_909 && smc_led(24));
    CHECK(smc_same(first, smc_target(0)));
    /* The other peer can control 808 without redirecting this peer. */
    smc_slot = 1;
    smc_page_set(2);
    CHECK(smc_page_now() == 2);
    CHECK(smc_drum_mix() == PART_808);
    CHECK(!smc_led(24));
    smc_slot = 0;
    CHECK(smc_page_now() == 1 && smc_led(24));
    /* Mixer PAGE+ changes only the mixer's page, not the keyboard page. */
    page_tap(0x63);
    CHECK(smc_page_now() == 2 && cur_page() == 2);
    CHECK(smc_drum_mix() == PART_808 && !smc_led(24));
    page_tap(0x62);
    CHECK(smc_page_now() == 1 && cur_page() == 2);
    /* A lock holds the exact physical targets while mixer navigation moves
     * lock_page; unlock then adopts that independent page. */
    page_tap(0x5D);
    CHECK(smc.locked);
    pref_t held_fader = smc_target(0), held_knob = smc_knob_target(0);
    page_tap(0x63);
    CHECK(smc.lock_page == 2 && smc_page_now() == 2);
    CHECK(smc_same(held_fader, smc_target(0)));
    CHECK(smc_same(held_knob, smc_knob_target(0)));
    *page_of(V_MIX) = 3;
    build_pages();
    CHECK(smc_page_now() == 2 && cur_page() == 3);
    CHECK(smc_same(held_fader, smc_target(0)));
    CHECK(smc_lock_toggle());
    CHECK(smc_page_now() == 2 && smc_drum_mix() == PART_808);
    CHECK(!smc_same(held_fader, smc_target(0)));
    /* Proven drum banks are independent and never collide with a fader. */
    for (int bank = 0; bank < 3; bank++) {
        page_tap(0x61);
        for (int i = 0; i < 8; i++) {
            pref_t k = smc_knob_target(i);
            if (k.kind == R_NONE) continue;
            for (int f = 0; f < 8; f++)
                CHECK(!smc_same(k, smc_target(f)));
            for (int j = 0; j < i; j++)
                CHECK(!smc_same(k, smc_knob_target(j)));
        }
    }
    printf("PASS independent SMC mixer pages, LEDs, two peers and target lock (%u checks)\n",checks);
    return 0;
}
