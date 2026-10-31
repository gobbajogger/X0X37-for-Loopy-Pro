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
    /* The unlocked mixer follows keyboard-only page changes from first use. */
    *page_of(V_MIX) = 3; build_pages();
    CHECK(cur_page() == 3 && smc_page_now() == 3 && smc_led(31));
    smc_page_set(0);
    CHECK(cur_page() == 0 && smc_page_now() == 0 && !smc_led(31));
    ui.sel[PART_909] = 0;
    ui.sel[PART_808] = 1;
    build_pages();
    smc_page_set(1);
    CHECK(cur_page() == 1 && smc_page_now() == 1);
    CHECK(smc_drum_mix() == PART_909 && smc_led(24));
    pref_t first = smc_target(0);
    *page_of(V_MIX) = 2; build_pages();
    CHECK(cur_page() == 2 && smc_page_now() == 2);
    CHECK(smc_drum_mix() == PART_808 && !smc_led(24) && smc_led(25));
    CHECK(!smc_same(first, smc_target(0)));
    /* Both unlocked peers read the keyboard page, including mixer-side moves. */
    smc_slot = 1;
    CHECK(smc_page_now() == 2);
    smc_page_set(1);
    CHECK(cur_page() == 1 && smc_page_now() == 1 && smc_led(24));
    smc_slot = 0;
    CHECK(smc_page_now() == 1 && smc_led(24));
    page_tap(0x63);
    CHECK(cur_page() == 2 && smc_page_now() == 2 && smc_led(25));
    page_tap(0x62);
    CHECK(cur_page() == 1 && smc_page_now() == 1 && smc_led(24));

    /* Pause locks the physical targets AND the peer's own display page.
     * Mixer PAGE+ changes lock_page, not the keyboard page or saved targets. */
    page_tap(0x5D);
    CHECK(smc.locked && smc.lock_view == V_MIX &&
          smc.lock_part == PART_909 && smc.lock_page == 1);
    CHECK(smc.lock_kit == PART_909 + 1 && smc.lock_bank == 0);
    pref_t held_fader = smc_target(0), held_knob = smc_knob_target(0);
    smc_slot = 1;
    smc_page_set(3);
    CHECK(cur_page() == 3 && smc_page_now() == 3 && smc_led(31));
    smc_slot = 0;
    CHECK(smc_page_now() == 1 && smc_led(24));
    CHECK(smc_same(held_fader, smc_target(0)));
    CHECK(smc_same(held_knob, smc_knob_target(0)));
    page_tap(0x63);
    /* Approved #737 peek: locked PAGE+ briefly shows the locked page on the
     * keyboard screen, then the timeout restores the keyboard page. */
    CHECK(smc.lock_page == 2 && smc_page_now() == 2);
    CHECK(smc_peek->on && cur_page() == 2);
    now_ms += SMC_PEEK_MS + 1; smc_poll();
    CHECK(!smc_peek->on && cur_page() == 3);
    CHECK(smc_drum_mix() == PART_808 && !smc_led(24) && smc_led(25));
    /* Lock only stops following the keyboard: the SMC's own PAGE+ retargets
     * faders and knobs to its locked page (808 MIX), then holds them there. */
    CHECK(!smc_same(held_fader, smc_target(0)) && smc_target(0).a == T_808);
    held_fader = smc_target(0); held_knob = smc_knob_target(0);
    /* A keyboard-only view change to PART/FX/GEN cannot replace the
     * locked MIX page's count, kit map, SELECT LED, or captured targets. */
    const int other_views[] = {V_PART, V_FX, V_GEN};
    for (unsigned vi = 0; vi < sizeof other_views / sizeof *other_views; vi++) {
        ui.part = PART_303A;
        set_view(other_views[vi]); build_pages();
        CHECK(smc_page_now() == 2 && smc.lock_pages > 3);
        CHECK(smc_drum_mix() == PART_808 && smc_led(25) && !smc_led(24));
        smc_page_set(3);
        CHECK(smc_page_now() == 3 && smc_led(31));
        CHECK(!smc_same(held_fader, smc_target(0))); /* SMC's own page 4 */
        smc_page_set(2);
        CHECK(smc_page_now() == 2 && smc_led(25));
        /* Even an empty live page table must not strand the locked mixer. */
        int keyboard_pages = pg.n;
        pg.n = 0;
        CHECK(smc_page_now() == 2 && smc_led(25));
        smc_page_set(3);
        CHECK(smc_page_now() == 3 && smc_led(31));
        smc_page_set(2);
        pg.n = keyboard_pages;
        CHECK(smc_same(held_fader, smc_target(0)));
    }
    ui.part = PART_909;
    set_view(V_MIX); build_pages();
    CHECK(smc_page_now() == 2 && smc_led(25));
    /* Peers can lock different pages and never overwrite one another's lock. */
    smc_slot = 1;
    CHECK(smc_lock_toggle() && smc.locked && smc.lock_page == 3);
    smc_slot = 0;
    CHECK(smc.locked && smc.lock_page == 2);
    *page_of(V_MIX) = 0; build_pages();
    CHECK(cur_page() == 0 && smc_page_now() == 2);
    CHECK(smc_same(held_fader, smc_target(0)));
    smc_slot = 1;
    CHECK(smc_page_now() == 3 && smc.lock_page == 3);
    CHECK(smc_lock_toggle() && smc_page_now() == 0);
    smc_slot = 0;
    CHECK(smc_lock_toggle() && smc_page_now() == 0);
    CHECK(!smc_same(held_fader, smc_target(0)));
    /* After unlock, changing the mixer's page changes the keyboard page. */
    smc_page_set(1);
    CHECK(smc_page_now() == 1 && cur_page() == 1);
    CHECK(smc_drum_mix() == PART_909 && smc_led(24));

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
    /* Short Pause press while locked: show the locked page for
     * SMC_PEEK_MS, then restore the keyboard page. Lock stays on. */
    setup(V_MIX, PART_909);
    *page_of(V_MIX) = 1; build_pages();
    page_tap(0x5D);
    CHECK(smc.locked && smc.lock_view == V_MIX && smc.lock_page == 1);
    now_ms += SMC_PEEK_MS + 1; smc_poll();
    *page_of(V_MIX) = 3; build_pages(); smc_peek_cancel();
    CHECK(cur_page() == 3 && !smc_peek->on);
    page_tap(0x5D);
    CHECK(smc.locked && smc_peek->on && ui.view == V_MIX && cur_page() == 1);
    ui_frame();
    now_ms += SMC_PEEK_MS - 100; smc_poll();
    CHECK(smc_peek->on && cur_page() == 1);
    now_ms += 101; smc_poll();
    CHECK(smc.locked && !smc_peek->on && cur_page() == 3);
    /* Long Pause press still unlocks and shows no peek. */
    page_note(0x90, 0x5D, 127);
    now_ms += 1000; smc_poll();
    page_note(0x80, 0x5D, 0);
    CHECK(!smc.locked && !smc_peek->on && cur_page() == 3);
    printf("PASS independent SMC mixer pages, LEDs, two peers and target lock (%u checks)\n",checks);
    return 0;
}
