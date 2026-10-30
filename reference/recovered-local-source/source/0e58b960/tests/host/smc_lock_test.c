/* SPDX-License-Identifier: GPL-3.0-only */
/* Production integration checks adapted from the reviewed PR150 suite. Routing runs through
 * the same fixture as smc_ui; hardware is replaced by a bounded output recorder.
 * Physical Pause bytes are retained-capture evidence for this owner preset.
 * These host tests establish dispatch/output bytes, never physical LED identity. */
#include <stdint.h>
#include <string.h>
static unsigned send_count, led_count, led_transport_count;
static uint8_t sent[256][3];
int plat_ble_smc_send(const uint8_t *m, uint8_t n)
{
    if (send_count < 256 && n == 3) memcpy(sent[send_count], m, 3);
    send_count++;
    if (n == 3 && m[0] == 0x90) {
        led_count++;
        if (m[1] >= 0x5D && m[1] <= 0x5F) led_transport_count++;
    }
    return 1;
}
#define SMC_UI_SEND_IMPLEMENTED 1
#define SMC_UI_MAIN smc_ui_baseline_main
#include "smc_ui_test.c"

/* Baseline replay alone supplies an absent-operation stub, so it compiles and
 * reaches the new target assertion. A compiler error never counts as negative. */
#ifdef SMC_LOCK_BASELINE
static int smc_lock_toggle(void) { return 1; }
static int smc_lock_button(int pressed) { (void)pressed; return 0; }
#endif

static void navigate(int view, int part)
{
    ui.part = (uint8_t)part;
    ui.mixsel = (uint8_t)part;
    set_view(view);
    build_pages();
    smc_ui_cancel();
}
static void reset_output(void)
{
    send_count = led_count = led_transport_count = 0;
    memset(sent, 0, sizeof sent);
}
static void test_drum_lock(void)
{
    for (int kit = 0; kit < 2; kit++) for (int bank = 0; bank < 2; bank++) {
        pref_t f[8], k[8];
        setup(V_SEQ, kit); ui.sel[kit] = (uint8_t)(bank * 8); build_pages();
        for (int i = 0; i < 8; i++) { f[i] = smc_target(i); k[i] = smc_knob_target(i); }
        CHECK(smc_lock_toggle());
        navigate(V_PART, kit ? PART_909 : PART_808);
        ui.sel[kit] = (uint8_t)(bank ? 0 : 8); build_pages();
        for (int i = 0; i < 8; i++) {
            CHECK(smc_same(f[i], smc_target(i))); /* original source fails here */
            CHECK(smc_same(k[i], smc_knob_target(i)));
            if (bank && i >= 3) CHECK(smc_target(i).kind == R_NONE);
        }
        pref_t r = k[0];
        pref_set(r, 48); int before = pref_get(r);
        screen_state_t ui_before = screen_state();
        knob(0, 1); CHECK(pref_get(r) > before); unchanged(ui_before);
        int muted = track_muted(kit, bank * 8);
        int foreign = track_muted(!kit, 0);
        message(0x90, 16, 127);
        CHECK(track_muted(kit, bank * 8) != muted);
        CHECK(track_muted(!kit, 0) == foreign);
        message(0x90, 8, 127);
        CHECK(smc.drum_solo[kit] & (1u << (bank * 8)));
        message(0x90, 8, 127); /* repeated held key cannot redirect */
        CHECK(smc.drum_solo[kit] == (1u << (bank * 8)));
        message(0x80, 8, 0); CHECK(!smc_project_active());
        message(0x80, 8, 0); CHECK(!smc_project_active());
        /* Button actions stay live; every captured continuous-control ref stays fixed. */
        int record_before = ui.rec;
        message(0x90, 0, 127);
        CHECK(ui.part == kit && ui.sel[kit] == bank * 8 && ui.rec != record_before);
        message(0x80, 0, 0);
        navigate(V_PART, kit ? PART_909 : PART_808);
        message(0x90, 24, 127);
        CHECK(ui.part == kit && ui.sel[kit] == bank * 8);
        message(0x80, 24, 0);
        navigate(V_MIX, kit); *page_of(V_MIX) = (uint8_t)(kit + 1);
        ui.sel[kit] = 2; build_pages();
        message(0x90, 0x2E, 127); message(0x80, 0x2E, 0); CHECK(ui.sel[kit] == 0);
        message(0x90, 0x2F, 127); message(0x80, 0x2F, 0); CHECK(ui.sel[kit] == 8);
        message(0x90, 0x30, 127); message(0x80, 0x30, 0); CHECK(ui.sel[kit] == 7);
        message(0x90, 0x31, 127); message(0x80, 0x31, 0); CHECK(ui.sel[kit] == 8);
        navigate(V_FX, kit); *page_of(V_FX) = 0; build_pages();
        message(0x90, 0x63, 127); message(0x80, 0x63, 0); CHECK(cur_page() == 1);
        message(0x90, 0x62, 127); message(0x80, 0x62, 0); CHECK(cur_page() == 0);
        for (int i = 0; i < 8; i++) {
            CHECK(smc_same(f[i], smc_target(i)));
            CHECK(smc_same(k[i], smc_knob_target(i)));
        }
        CHECK(smc_lock_toggle());
        CHECK(smc_same(smc_target(0), smc_home_target(0)));
    }
    puts("PASS lock drum identity: both kits/banks, empty strips, frozen edits/mute/Solo and live button navigation");
}
static void test_engine_lock_and_clip(void)
{
    setup(V_PART, PART_303A);
    /* Capture the actual Cutoff identity regardless of SOUND page order;
     * Engine selectors have a different range and cannot prove this edit. */
    pref_t r = find_ref(T_303, 0, "Cutoff");
    pref_t other = find_ref(T_303, 1, "Cutoff");
    CHECK(r.kind == R_ENG && other.kind == R_ENG);
    int cutoff_page = -1, cutoff_knob = -1;
    for (int page = 0; page < pg.n; page++) for (int k = 0; k < 4; k++)
        if (smc_same(pg.r[page][k], r)) { cutoff_page = page; cutoff_knob = k; }
    CHECK(cutoff_page >= 0 && cutoff_knob >= 0);
    *page_of(V_PART) = (uint8_t)cutoff_page; build_pages();
    CHECK(smc_same(smc_knob_target(cutoff_knob), r));
    CHECK(pref_desc(r)->max >= 85 && pref_desc(other)->max >= 85);
    pref_set(r, 40); pref_set(other, 85);
    CHECK(pref_get(r) == 40 && pref_get(other) == 85);
    CHECK(smc_lock_toggle());
    navigate(V_PART, PART_303B);
    ui.sel[0] = 10; ui.sel[1] = 9; ui.pbank = ui.spage = 1;
    screen_state_t ui_before = screen_state();
    knob(cutoff_knob, 1); CHECK(pref_get(r) > 40); CHECK(pref_get(other) == 85); unchanged(ui_before);
    clip_grid_mode = 1; ck.pg = 1; clip_dirty = 1;
    unsigned field = CK_PAGE_FIELDS[0][cutoff_knob]; fields[field] = 0;
    int v = pref_get(r); knob(cutoff_knob, 1);
    CHECK(pref_get(r) > v); CHECK(fields[field] == 0);
    CHECK(smc_lock_toggle()); /* release always works, including from clip context */
    knob(cutoff_knob, 1); CHECK(fields[field] == 1);
    CHECK(!smc_lock_toggle()); /* no global clip-stage identity is guessed */
    clip_grid_mode = 0;
    puts("PASS engine/clip boundaries: original part target survives navigation; unlocked clip fields resume");
}
static void test_pattern_lock(void)
{
    /* These refs contain an implicit live pattern in the original UI. Locking
     * must bind the original slot as well as the visible parameter identity. */
    setup(V_GEN, PART_303A); seq.ppat[PART_303A] = 2; build_pages();
    proj.pat[2].bass[0].gen.density = 30;
    proj.pat[7].bass[0].gen.density = 90;
    proj.pat[8].bass[1].gen.density = 70;
    CHECK(smc_lock_toggle());
    seq.ppat[PART_303A] = 7; seq.ppat[PART_303B] = 8;
    navigate(V_GEN, PART_303B);
    screen_state_t ui_before = screen_state();
    knob(0, 1);
    CHECK(proj.pat[2].bass[0].gen.density > 30);
    CHECK(proj.pat[7].bass[0].gen.density == 90);
    CHECK(proj.pat[8].bass[1].gen.density == 70);
    unchanged(ui_before);
    CHECK(smc_lock_toggle());
    knob(0, 1); CHECK(proj.pat[8].bass[1].gen.density > 70);

    setup(V_SEQ, PART_BRK); seq.ppat[PART_BRK] = 3; build_pages();
    proj.pat[3].brk.set[BRK_COMPLEX] = 30;
    proj.pat[5].brk.set[BRK_COMPLEX] = 80;
    CHECK(smc_lock_toggle()); seq.ppat[PART_BRK] = 5;
    navigate(V_FX, PART_909); knob(0, 1);
    CHECK(proj.pat[3].brk.set[BRK_COMPLEX] > 30);
    CHECK(proj.pat[5].brk.set[BRK_COMPLEX] == 80);
    CHECK(smc_lock_toggle());

    setup(V_SEQ, PART_303A); seq.ppat[PART_303A] = 1; build_pages();
    proj.pat[1].bass[0].len = 16; proj.pat[6].bass[0].len = 32;
    CHECK(smc_lock_toggle()); seq.ppat[PART_303A] = 6;
    navigate(V_SEQ, PART_303B); knob(0, 1);
    CHECK(proj.pat[1].bass[0].len > 16); CHECK(proj.pat[6].bass[0].len == 32);

    setup(V_PART, PART_BRK); CHECK(engine_brk_nslots() > 1);
    seq.ppat[PART_BRK] = 2; build_pages();
    proj.pat[2].brk.slot_a = 0; proj.pat[7].brk.slot_a = 0;
    CHECK(smc_lock_toggle()); seq.ppat[PART_BRK] = 7;
    navigate(V_PART, PART_303A); knob(0, 1);
    CHECK(proj.pat[2].brk.slot_a == 1); CHECK(proj.pat[7].brk.slot_a == 0);
    puts("PASS pattern identity: GEN, BRK and bass sequence fields stay on captured pattern slots");
}
static void test_song_lock(void)
{
    setup(V_SONG, PART_303A); song_fill(6); ui.song_sel = 2; build_pages();
    song()->bar[2].pat[0] = 3; song()->bar[5].pat[0] = 9;
    CHECK(smc_lock_toggle()); ui.song_sel = 5;
    CHECK(smc_knob_target(0).kind == R_NONE); /* cursor is navigation, never a frozen parameter */
    screen_state_t ui_before = screen_state();
    knob(1, 1); CHECK(song()->bar[2].pat[0] == 4); CHECK(song()->bar[5].pat[0] == 9);
    unchanged(ui_before);
    song()->len = 2; CHECK(smc_knob_target(1).kind == R_NONE);
    knob(1, 1); CHECK(song()->len == 2); /* deleted bar cannot silently regrow */
    CHECK(smc_lock_toggle());
    setup(V_SONG, PART_303A); song_fill(6); ui.song_sel = 3; build_pages();
    CHECK(smc_lock_toggle()); ui.song_sel = 0;
    run_action(ACT_SONG_INS, 0); run_action(ACT_SONG_DEL, 0);
    CHECK(song()->len == 7); CHECK(smc_knob_target(1).kind == R_NONE);
    uint8_t pat_before = song()->bar[3].pat[0];
    knob(1, 1); CHECK(song()->bar[3].pat[0] == pat_before);
    CHECK(smc_lock_toggle());
    setup(V_SONG, PART_303A); song_fill(6); ui.song_sel = 3;
    *page_of(V_SONG) = 1; build_pages(); song()->bar[3].mute = 2u << 5;
    CHECK(smc_knob_target(3).kind == R_SREP); CHECK(smc_lock_toggle());
    ui.song_sel = 0; run_action(ACT_SONG_INS, 0);
    CHECK(smc_knob_target(3).kind == R_NONE);
    uint8_t mute_before = song()->bar[3].mute;
    knob(3, 1); CHECK(song()->bar[3].mute == mute_before);
    CHECK(smc_lock_toggle());
    ui.song_sel = 3; build_pages(); CHECK(smc_lock_toggle());
    song_fill(song()->len); CHECK(smc_knob_target(3).kind == R_NONE);
    CHECK(smc_lock_toggle());
    ui.song_sel = 3; build_pages(); CHECK(smc_lock_toggle());
    undo_start(); song()->bar[3].pat[0] ^= 1u; undo_commit(&undo); do_undo(0);
    CHECK(smc_knob_target(3).kind == R_NONE);
    puts("PASS song identity: captured bar, parked cursor and trimmed-bar invalidation");
}
static void test_solo_lock_boundaries(void)
{
    for (int kit = 0; kit < 2; kit++) {
        setup(V_SEQ, kit); CHECK(smc_lock_toggle()); navigate(V_PART, PART_303A);
        for (int note = 13; note <= 15; note++) {
            message(0x90, note, 127);
            CHECK(smc.drum_solo[kit] == (1u << (note - 8)));
            message(0x80, note, 0); CHECK(!smc_project_active());
        }
        setup(V_SEQ, kit); ui.sel[kit] = 0; build_pages();
        message(0x90, 8, 127); CHECK(smc.drum_solo[kit] == 1u);
        ui.sel[kit] = 8; build_pages(); CHECK(smc_lock_toggle());
        message(0x80, 8, 0); CHECK(!smc_project_active());
        message(0x90, 8, 127); CHECK(smc.drum_solo[kit] == (1u << 8));
        message(0x80, 8, 0); CHECK(!smc_project_active());
    }
    puts("PASS Solo boundaries: bank-one Solo6–8 remain valid; key-up clears original bank before/after lock");
}
static void test_edges_and_global_scope(void)
{
    setup(V_SEQ, PART_909); ui.sel[0] = 8; build_pages();
    pref_t frozen = smc_target(0);
    CHECK(smc_lock_button(1)); navigate(V_PART, PART_303A);
    CHECK(smc_same(frozen, smc_target(0)));
    CHECK(!smc_lock_button(1)); CHECK(!smc_lock_button(1));
    CHECK(smc_same(frozen, smc_target(0)));
    CHECK(!smc_lock_button(0)); CHECK(!smc_lock_button(0));
    CHECK(!smc_lock_button(1)); now_ms += 900; smc_poll();
    CHECK(!smc.locked); CHECK(smc_same(smc_target(0), smc_home_target(0)));
    CHECK(!smc_lock_button(1)); CHECK(!smc_lock_button(0));
    CHECK(smc_lock_button(1)); /* lock HOME targets while UI is on a sound page */
    navigate(V_SEQ, PART_808);
    smc_ble_link(0); smc_poll(); smc_ble_link(1); smc_poll();
    CHECK(smc_same(smc_target(0), smc_home_target(0)));
    CHECK(!smc_lock_button(1)); now_ms += 900; smc_poll(); /* reconnect rearms a new hold */
    CHECK(!smc.locked);
    CHECK(smc_target(0).a == T_808);

    setup(V_SEQ, PART_909); seq.ppat[PART_909] = 1;
    CHECK(smc_lock_toggle()); navigate(V_SEQ, PART_303A); seq.ppat[PART_303A] = 4;
    message(0x90, 0x5C, 127);
    CHECK(seq_cue_of(&seq, PART_909) == 2); CHECK(seq_cue_of(&seq, PART_303A) == -1);
    CHECK(seq.ppat[PART_909] == 1); CHECK(seq.ppat[PART_303A] == 4);
    seq.ppat[PART_909] = 2; /* consume cue as the sequencer does at its boundary */
    message(0x90, 0x5B, 127); CHECK(seq_cue_of(&seq, PART_909) == 1);
    int rec = ui.rec;
    message(0x90, 0x5F, 127); CHECK(ui.rec != rec); CHECK(smc.rec_part == PART_303A);
    puts("PASS edge/global scope: held duplicates, release/reconnect, captured-part pattern cues, current-part global REC");
}
static void test_pickup_and_display(void)
{
    setup(V_SEQ, PART_909); ui.sel[0] = 0; build_pages();
    pref_t r = smc_target(0); pref_set(r, 80); smc_frame();
    fader(0, smc_scaled(r)); CHECK(smc.picked & 1u);
    CHECK(smc_lock_toggle()); CHECK(!(smc.picked & 1u));
    navigate(V_SEQ, PART_808); ui.sel[1] = 8; build_pages();
    pref_t next = find_ref(T_808, 8, "Level"); pref_set(next, 40);
    /* Move the target above the last physical sample: the next downward
     * movement genuinely does not cross it, even with retained history. */
    pref_set(r, 96); smc_frame();
    int original = pref_get(r), nextval = pref_get(next);
    screen_state_t ui_before = screen_state();
    fader(0, 0); CHECK(pref_get(r) == original); CHECK(!(smc.picked & 1u));
    CHECK(smc_ui_active()); ui_frame(); unchanged(ui_before); shot("locked-context");
    now_ms += 1199; ui_frame(); CHECK(smc_ui_active());
    now_ms++; ui_frame(); CHECK(!smc_ui_active()); unchanged(ui_before);
    fader(0, smc_scaled(r)); CHECK(smc.picked & 1u);
    fader(0, 127); CHECK(pref_get(r) >= original); CHECK(pref_get(next) == nextval);
    pref_set(r, 25); smc_frame(); CHECK(!(smc.picked & 1u));
    CHECK(smc_lock_toggle()); CHECK(!(smc.picked & 1u)); CHECK(smc.seen & 1u);
    CHECK(smc_same(next, smc_target(0)));
    fader(0, 127); CHECK(pref_get(next) == nextval); CHECK(!(smc.picked & 1u));
    fader(0, smc_scaled(next)); CHECK(smc.picked & 1u);
    CHECK(smc_lock_toggle()); fader(0, 0); CHECK(smc_ui_active());
    held_btn = 1u << B_FX; input(); CHECK(!smc_ui_active());
    held_btn = 0; input(); CHECK(ui.view == V_FX);
    fader(0, 1); CHECK(smc_ui_active());
    ask(ACT_SONG_CLR, 0, "TEST", "KEEP LOCK"); CHECK(!smc_ui_active());
    CHECK(smc_target(0).kind == R_NONE); CHECK(smc_knob_target(0).kind == R_NONE);
    button_tap(B_HOME); CHECK(ui.overlay != O_ASK); CHECK(smc_same(next, smc_target(0)));
    puts("PASS pickup/display: invalidated transitions, frozen edit, remote edit invalidation, exact 1200ms return");
}
static void test_physical_pause_input_and_output(void)
{
    setup(V_SEQ, PART_909); ui.sel[0] = 8; build_pages();
    pref_t r = smc_target(0);
    seq_start(&seq);
    screen_state_t before = screen_state();
    message(0x90, 0x5D, 127);
    CHECK(smc.locked); CHECK(smc.lock_down);
    CHECK(seq.req_start && !seq.req_stop); CHECK(!smc_ui_active());
    CHECK(ui.view == before.view && ui.overlay == before.overlay && !memcmp(ui.page, before.page, sizeof ui.page));
    navigate(V_PART, PART_303A);
    for (int i = 0; i < 7; i++) message(0x90, 0x5D, 127);
    CHECK(smc.locked); CHECK(smc_same(r, smc_target(0))); CHECK(!seq.req_stop);
    message(0x90, 0x5D, 0); message(0x80, 0x5D, 0);
    CHECK(smc.locked && !smc.lock_down); CHECK(!smc_ui_active());
    message(0x90, 0x5D, 127); CHECK(smc.locked); now_ms += 900; smc_poll(); CHECK(!smc.locked); CHECK(!seq.req_stop);
    message(0x80, 0x5D, 0); CHECK(!smc.locked);

    /* Unrelated controls and unselected DAW channels cannot capture a target. */
    message(0x90, 53, 127); message(0x80, 53, 0);
    message(0x91, 0x5D, 127); message(0x81, 0x5D, 0);
    CHECK(!smc.locked);
    proj.set.bt |= PROJ_BT_SMC_USER;
    CHECK(!smc_midi_host(9u | 0x90u << 8 | 0x5Du << 16 | 127u << 24));
    CHECK(!smc_midi_host(9u | 0x9Fu << 8 | 0x5Du << 16 | 127u << 24)); smc_poll();
    CHECK(!smc.locked && !smc.lock_down);
    CHECK(!smc_midi_host(8u | 0x8Fu << 8 | 0x5Du << 16)); smc_poll();
    CHECK(!smc.locked && !smc.lock_down);
    message(0x90, 0x5D, 127); message(0x80, 0x5D, 0);
    CHECK(smc.locked && !smc.lock_down); /* ordinary DAW capture remains */

    /* The old User-preset MMC command has no captured press/release evidence. */
    const uint8_t stop[6] = {0xF0, 0x7F, 0x7F, 6, 34, 0xF7};
    unsigned start_before=seq.req_start, stop_before=seq.req_stop;
    smc_ble_midi(stop, sizeof stop); smc_poll();
    CHECK(seq.req_start==start_before && seq.req_stop==stop_before); CHECK(smc.locked);
    seq.req_start=0; seq.req_stop=1; /* establish original Play-toggle premise via fixture, not retired MMC */
    message(0x90, 0x5E, 127); CHECK(seq.req_start);
    message(0x80, 0x5E, 0); CHECK(seq.req_start);
    message(0x90, 0x5E, 127); CHECK(seq.req_stop);
    message(0x80, 0x5E, 0); CHECK(smc.locked);

    /* Existing output route only: NoteOn 90 5D, solid captured / blink following. */
    smc_frame(); reset_output();
    for (int i = 0; i < 100; i++) { now_ms += 20; smc_frame(); }
    CHECK(send_count == 0);
    CHECK(smc_led(0x5D));
    smc_ble_link(0); smc_poll(); CHECK(smc.locked);
    smc_ble_link(1); smc_poll(); CHECK(smc.locked);
    reset_output(); smc_frame(); CHECK(led_count == 43); CHECK(led_transport_count == 3);
    reset_output(); smc_frame(); CHECK(send_count == 0);
    message(0x90, 0x5D, 127); CHECK(smc.locked); now_ms += 900; smc_poll(); CHECK(!smc.locked); CHECK(!smc_ui_active());
    message(0x90, 0x5D, 0);
    seq.ext_now = smc.beat_at + smc.beat_period / 2u; smc_frame(); reset_output();
    seq.ext_now = smc.beat_at + smc.beat_period; smc_frame();
    CHECK(send_count == 1 && sent[0][0] == 0x90 && sent[0][1] == 0x5D && sent[0][2] == 127);
    reset_output(); seq.ext_now += smc.beat_period / 2u; smc_frame();
    CHECK(send_count == 1 && sent[0][0] == 0x90 && sent[0][1] == 0x5D && sent[0][2] == 0);
    reset_output(); smc_frame(); CHECK(send_count == 0);
    puts("PASS captured Pause dispatch: duplicate/release edges, no navigation/overlay, channel isolation, retired MMC inert, Play toggle, lock persistence and LED route");
}
static void test_guard_reset(void)
{
    setup(V_HOME, PART_303A); ui.overlay = O_ASK;
    CHECK(!smc_lock_toggle()); ui.overlay = O_NONE;
    perf.test = 0; CHECK(!smc_lock_toggle()); perf.test = PT_IDLE;
    CHECK(ui_project_begin() == 0); CHECK(!smc_lock_toggle()); ui_project_end(0);
    CHECK(smc_lock_toggle());
    smc_project_reset(); navigate(V_SEQ, PART_909);
    CHECK(smc_drum_mix() == PART_909); /* reset removes old home context */
    CHECK(smc_target(0).a == T_909);
    puts("PASS guards/reset: confirmation and project ownership reject capture; project reset releases context");
}

static void press(unsigned note)
{
    message(0x90, note, 127);
    message(0x80, note, 0);
}
static void test_select_and_play(void)
{
    setup(V_FX, PART_303A);
    message(0x90, 27, 127); CHECK(smc.fx_down == (1u << SMCFX_303B));
    message(0x80, 27, 0); CHECK(!smc.fx_down);
    for (int kit = 0; kit < NKIT; kit++) {
        setup(V_HOME, PART_303A);
        int rec = ui.rec;
        press(24 + kit); CHECK(ui.part == kit); CHECK(ui.view == V_PART);
        CHECK(!smc_ui_active() && !smc.fx_down);
        CHECK(ui.rec == rec); CHECK(!seq.req_start && !seq.req_stop);
        setup(V_MIX, kit); *page_of(V_MIX) = (uint8_t)(kit + 1); build_pages();
        CHECK(drum_mix_page() == kit); CHECK(ui.rec == rec);
        unsigned selected = ui.sel[kit];
        unsigned other = selected == 0 ? 1 : 0;
        press(24 + other); CHECK(ui.sel[kit] == other); CHECK(drum_mix_page() == kit);
        message(0x90, 24 + other, 127); CHECK(cur_page() == 0); CHECK(!smc_ui_active());
        message(0x90, 24 + other, 127); CHECK(cur_page() == 0); /* held repeat is one gesture */
        message(0x80, 24 + other, 0);
        *page_of(V_MIX) = (uint8_t)(kit + 1); build_pages();
        CHECK(drum_mix_page() == kit);
        CHECK(smc_lock_toggle());
        pref_t frozen = smc_target(0), knob_target = smc_knob_target(0);
        press(24 + other); CHECK(ui.part == kit && ui.sel[kit] == other);
        CHECK(ui.view == V_MIX && cur_page() == 0); CHECK(!smc_ui_active());
        CHECK(smc_same(frozen, smc_target(0))); CHECK(smc_same(knob_target, smc_knob_target(0)));
        CHECK(ui.rec == rec); CHECK(!seq.req_start && !seq.req_stop);
        CHECK(smc_lock_toggle());
    }
    setup(V_HOME, PART_909);
    press(0x5E); CHECK(seq.req_start); CHECK(!seq.req_stop);
    press(0x5E); CHECK(seq.req_stop); CHECK(!seq.req_start);
    seq.req_stop = 0; seq.playing = 1;
    message(0x90, 0x5E, 127); CHECK(seq.req_stop);
    message(0x90, 0x5E, 127); CHECK(!seq.req_start); /* held repeat cannot restart */
    message(0x80, 0x5E, 0); seq.playing = 0;
    press(0x5E); CHECK(seq.req_start);
    unsigned pending_start = seq.req_start, pending_stop = seq.req_stop;
    press(0x5D); CHECK(smc.locked && seq.req_start == pending_start && seq.req_stop == pending_stop); /* captured Pause locks, never stops */
    puts("PASS Select/Play: first select, same-part drum mixer, same-sound return, held edges, frozen lock and Play toggle");
}


static void test_lost_edge_release(void)
{
    for (int host = 0; host < 2; host++) for (int edge = 0; edge < 3; edge++) for (int noteoff = 0; noteoff < 2; noteoff++) {
        setup(V_HOME, PART_303A);
        int select = edge == 1, pause = edge == 2;
        unsigned note = pause ? 0x5Du : select ? 24u : 0x5Eu;
        press(note); /* settle into ordinary selected/playing request state */
        if (select) CHECK(ui.part == PART_909); /* ordinary SELECT outside FX chooses its source */
        else if (!pause) { seq.req_start = 0; seq.playing = 1; }
        message(0x90, note, 127); /* leave held */
        CHECK(pause ? smc.lock_down != 0 : select ? smc.select_down != 0 : smc.play_down != 0);
        if (host) {
            proj.set.bt |= PROJ_BT_SMC_USER;
            for (unsigned i = 0; i < SMC_Q; i++) CHECK(!smc_midi_host(0x0Bu | 0xBFu << 8 | 16u << 16));
            CHECK(!smc_midi_host((noteoff ? 0x08u : 0x09u) | (noteoff ? 0x8Fu : 0x9Fu) << 8 | note << 16));
            CHECK(pause ? smc.lock_down != 0 : select ? smc.select_down != 0 : smc.play_down != 0);
        }
        {
            uint8_t filler[3] = {0xB0, 16, 0}, release[3] = {noteoff ? 0x80 : 0x90, (uint8_t)note, 0};
            for (unsigned i = 0; i < SMC_Q; i++) smc_ble_midi(filler, 3);
            smc_ble_midi(release, 3);
        }
        smc_poll();
        CHECK(pause ? smc.lock_down == 0 : select ? smc.select_down == 0 : smc.play_down == 0);
        if (pause) { CHECK(smc.locked); message(0x90, note, 127);
            now_ms += 900; smc_poll(); CHECK(!smc.locked); message(0x80, note, 0); }
        else if (select) { press(note); CHECK(cur_page() == 0); }
        else { seq.req_stop = 0; seq.playing = 1; press(note); CHECK(seq.req_stop); }
    }
    puts("PASS dropped release recovery: Pause, Play and SELECT rearm after full BLE queues, with/without inert retired-host traffic");
}

static void test_dropped_pause_press(void)
{
    for (int host = 0; host < 2; host++) {
        setup(V_SEQ, PART_808);
        uint8_t filler[3] = {0xB0, 16, 0}, down[3] = {0x90, 0x5D, 127};
        if (host) {
            proj.set.bt |= PROJ_BT_SMC_USER;
            for (unsigned i = 0; i < SMC_Q; i++) CHECK(!smc_midi_host(0x0Bu | 0xBFu << 8 | 16u << 16));
            CHECK(!smc_midi_host(9u | 0x9Fu << 8 | 0x5Du << 16 | 127u << 24));
            CHECK(!smc.locked && !smc.lock_down);
        }
        {
            for (unsigned i = 0; i < SMC_Q; i++) smc_ble_midi(filler, 3);
            smc_ble_midi(down, 3);
        }
        smc_poll(); CHECK(!smc.locked && !smc.lock_down);
        message(0x90, 0x5D, 0); CHECK(!smc.locked);
        press(0x5D); CHECK(smc.locked); CHECK(!smc.lock_down);
    }
    puts("PASS dropped Pause press: no phantom capture; following release is inert and next gesture captures");
}

int main(void)
{
    test_dropped_pause_press(); test_lost_edge_release(); test_select_and_play(); test_drum_lock(); test_engine_lock_and_clip(); test_pattern_lock(); test_song_lock();
    test_solo_lock_boundaries(); test_edges_and_global_scope(); test_pickup_and_display();
    test_physical_pause_input_and_output(); test_guard_reset();
    printf("smc_lock: PASS (%u checks; captured Pause input route; physical LED identity unverified)\n", checks);
    return 0;
}
