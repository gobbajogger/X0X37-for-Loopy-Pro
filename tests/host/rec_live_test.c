/* SPDX-License-Identifier: GPL-3.0-only */
/* Live recording on the whole app (the host simulator, host/x0x_host.c without its script main):
 *  - 303: ENV or LFO held with a key writes an accent or a slide, as step input does. The release of
 *    ENV or LFO does not open the FX or MIX view, and the next keys still record (a73).
 *  - 909: after a Start with the external clock already running, a hit before the first tick goes to
 *    step 1, not to the step of the last run (a73).
 *  - 909: a live hit records on the nearest step; just before the loop start: step 1 (a73).
 * Build: see tests/run_tests.sh (needs build/gen from host/build_host.sh). */
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE
#endif
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main

#if BOARD_SMK37
static uint8_t test_clip_mode, test_clip_busy;
uint8_t smk_clip_mode_on(void) { return test_clip_mode; }
uint8_t smk_padcal_on(void) { return 0; }
uint8_t smk_padcal_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void smk_padcal_enter(void) {}
uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h) { (void)w; (void)h; return 0; }
void smk_clip_lcd_force(void) {}
uint8_t smk_clip_wait(void) { return 0; }
uint8_t smk_ck_ext(void) { return 0; }
uint8_t smk_ck_flash_busy(void) { return test_clip_busy; }
uint8_t smk_ck_knobs_owned(void) { return 0; }
uint8_t smk_ck_panel(uint32_t *last, uint32_t force) { (void)last; (void)force; return 0; }
uint8_t smk_clip_history_button(uint8_t button, uint8_t pressed, uint8_t patch_held)
{ (void)button; (void)pressed; (void)patch_held; return 0; }
uint8_t smk_clip_transport_button(uint8_t button, uint8_t pressed)
{ (void)button; (void)pressed; return 0; }
void smk_play_note_button(uint8_t pressed, uint8_t home_held) { (void)pressed; (void)home_held; }
void smk_apc_transport_led_set(uint8_t note, uint8_t velocity) { (void)note; (void)velocity; }
uint8_t smk_apc_transport_led_state(uint8_t note) { (void)note; return 0; }
void smk_apc_clock_tick(void) {}
void smk_apc_clock_start(void) {}
void smk_apc_clock_position(uint32_t sixteenths) { (void)sixteenths; }
void smk_ck_midi_stop(void) {}
int smc_connected(void) { return 0; }
/* Disconnected SMC cannot own or draw the temporary control screen. */
int smc_ui_active(void) { return 0; }
void smc_ui_cancel(void) {}
void smc_ui_draw(void) {}
/* This fixture models disconnected SMC; project_smc_runtime_test.c covers
 * the production SMC project-active/reset implementation. */
int smc_project_active(void) { return 0; }
void smc_project_reset(void) {}
void smc_frame(void) {}
void smc_poll(void) {}
int smc_midi_host(uint32_t pkt) { (void)pkt; return 0; }
#endif

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("  FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* MIDI clock as Bluetooth MIDI gives it: the ticks of one connection event come together */
static int clk_on;
static double clk_tick_ms, clk_next_tick, clk_ev_ms, clk_next_ev;
static uint32_t clk_sent;
static void midi3(int s, int a, int b)
{
    uint32_t cin = s >= 0xF0 ? 0x0Fu : (uint32_t)s >> 4;
    min_q[mi_w++ % MQ] = cin | (uint32_t)s << 8 | (uint32_t)a << 16 | (uint32_t)b << 24;
}
static void pump(void)
{
    if (!clk_on)
        return;
    while (clk_next_ev <= now_ms) {
        while (clk_next_tick <= clk_next_ev) {
            midi3(0xF8, 0, 0);
            clk_sent++;
            clk_next_tick += clk_tick_ms;
        }
        clk_next_ev += clk_ev_ms;
    }
}
static void tick(uint32_t ms)
{
    while (ms--) {
        pump();
        run_ms(1);
    }
}
static void btn(int b, int down)
{
    if (down)
        held_btn |= 1u << b;
    else
        held_btn &= ~(1u << b);
    tick(1);
}
static void key(int k, int down)
{
    if (down)
        held_keys |= 1u << k;
    else
        held_keys &= ~(1u << k);
    tick(7); /* consume at least one production 256-frame audio block */
}
static void tap_key(int k, uint32_t ms) { key(k, 1); tick(ms); key(k, 0); }

/* wait until track t plays step s (0-based), then ms more */
static void at_step(int t, int s, uint32_t ms)
{
    uint32_t guard = 0;
    while (eng_step[t] == s && guard++ < 10000)       /* (a new pass of that step) */
        tick(1);
    while (eng_step[t] != s && guard++ < 10000)
        tick(1);
    tick(ms);
}

static bstep_t *bst(int s) { return &proj.pat[seq.ppat[PART_303A]].bass[0].step[s]; }

/* End-to-end FREE PLAY: measured key velocity and MIDI velocity reach the audio. */
static void test_303_freeplay_velocity(void)
{
    float peaks[2][2];
    int source, strike;
    ui.part = PART_303A;
    set_view(V_PART);
    ui.freeplay[0] = 1;
    build_pages();
    /* Measure the voice without factory RAT saturation, compression or echo tails. */
    engine_set(T_303, 0, BASS303_DRIVE, 0);
    engine_set(T_303, 0, BASS303_VOLUME, 96);
    engine_set(T_303, 0, BASS303_RESO, 0);
    engine_set(T_303, 0, BASS303_CUTOFF, 100);
    engine_set(T_303, 0, BASS303_ENVMOD, 0);
    engine_set(T_MIX, PART_303A, MX_DLY, 0);
    engine_set(T_MIX, PART_303A, MX_REV, 0);
    engine_set(T_MST, 0, MST_COMP1, 0);
    for (source = 0; source < 2; source++) {
        for (strike = 0; strike < 2; strike++) {
            int velocity = strike ? 90 : 20; /* both below MIDI's accent threshold */
            tick(1000);
            peak_out = 0.0f;
            if (source)
                midi3(0x90 | seq.ch[TRK_BASS0], kbd_note(10), velocity);
            else {
                key_velocity[10] = (uint8_t)velocity;
                key(10, 1);
            }
            tick(150);
            peaks[source][strike] = peak_out;
            if (source)
                midi3(0x80 | seq.ch[TRK_BASS0], kbd_note(10), 0);
            else
                key(10, 0);
        }
        CHECK(peaks[source][0] > 0.000001f && peaks[source][1] > peaks[source][0] * 1.5f,
              "FREE PLAY %s responds to velocity: soft %f hard %f", source ? "MIDI" : "keys",
              (double)peaks[source][0], (double)peaks[source][1]);
    }
    key_velocity[10] = 0;
    ui.freeplay[0] = 0;
    engine_apply_sound(&proj.sound);
    tick(1000);
}

static void test_303_key_release(void)
{
    ui.part = PART_303A;
    set_view(V_PART);
    ui.freeplay[0] = 1;
    build_pages();

    key(10, 1);
    tick(10);
    CHECK(engine_midi_held() & 1, "303 A key sounds before octave change");
    btn(B_OCTUP, 1);
    btn(B_OCTUP, 0);
    CHECK(engine_midi_held() & 1, "303 A key continues through octave change");
    key(10, 0);
    tick(10);
    CHECK(!(engine_midi_held() & 1), "303 A key releases after octave change");

    key(10, 1);
    tick(10);
    CHECK(engine_midi_held() & 1, "303 A key sounds before part change");
    enc_acc[EN_ALGO] += 1;
    tick(1);
    CHECK(ui.part == PART_303B, "ALGORITHM selects 303 B while A key is held");
    key(10, 0);
    tick(10);
    CHECK(!(engine_midi_held() & 1), "303 A key releases while 303 B is selected");

    ui.part = PART_303A;
    key(10, 1);
    key(12, 1);                                      /* 303 is monophonic: the later key owns the gate */
    tick(10);
    key(10, 0);
    tick(10);
    CHECK(engine_midi_held() & 1, "release of older key leaves later note sounding");
    key(12, 0);
    tick(10);
    CHECK(!(engine_midi_held() & 1), "later key releases its note");

    key(10, 1);
    tick(10);
    pref_set(PR(R_FREEPLAY, 0, 0, 0), 0);         /* SEQ > KEYS > FREE PLAY switched off */
    tick(10);
    CHECK(!(engine_midi_held() & 1), "free-play exit ends held note");
    key(10, 0);

    btn(B_REC, 1);
    btn(B_REC, 0);
    CHECK(ui.rec, "record turns on for the 303 keybed");
    key(10, 1);
    tick(10);
    btn(B_REC, 1);
    btn(B_REC, 0);
    CHECK(!ui.rec, "record turns off while key is held");
    key(10, 0);
    tick(10);
    CHECK(!(engine_midi_held() & 1), "held 303 key releases after record exit");

#if BOARD_SMK37
    ui.freeplay[0] = 1;
    key(10, 1);
    tick(10);
    CHECK(engine_midi_held() & 1, "303 A key sounds before clip entry");
    smk_clip_enter_cleanup();                     /* same cleanup used by the knob-bank mode switch */
    test_clip_mode = 1;
    tick(10);
    CHECK(!(engine_midi_held() & 1), "clip entry ends held 303 note");
    key(10, 0);
    test_clip_mode = 0;
#endif
    ui.freeplay[0] = 0;
    printf("  303 keyboard: octave and part changes release the original key; later key owns gate: checked\n");
}

static void test_303_step_preview_lifecycle(void)
{
    bstep_t *a, *b;
    int white = WHITE_KEY[0];
    ui.part = PART_303A;
    ui.rec = 0;
    ui.freeplay[0] = ui.freeplay[1] = 0;
    set_view(V_PART);
    a = &proj.pat[seq.ppat[PART_303A]].bass[0].step[0];
    b = &proj.pat[seq.ppat[PART_303B]].bass[1].step[0];
    a->note = 48;
    a->flags = G_NOTE;
    b->note = 55;
    b->flags = G_NOTE;

    key(white, 1);
    tick(10);
    CHECK(engine_midi_held() & 1, "303 A step preview starts while its key is held");
    enc_acc[EN_ALGO] = 1;                              /* change A to B with the key still down */
    tick(20);
    CHECK(ui.part == PART_303B, "ALGORITHM selected 303 B");
    CHECK(!(engine_midi_held() & 1), "303 A preview ended on part change");
    key(white, 0);
    CHECK(b->flags == G_NOTE && b->note == 55, "303 B step survives the old key-up");
    CHECK(a->flags == G_NOTE, "303 A step survives navigation without a tap");

    ui.part = PART_303A;
    set_view(V_PART);
    key(white, 1);
    tick(2);
    btn(B_FX, 1);
    btn(B_FX, 0);
    tick(20);
    CHECK(ui.view == V_FX && !(engine_midi_held() & 1), "FX closes the held step preview");
    key(white, 0);
    CHECK(a->flags == G_NOTE, "view change does not turn the old step off");

    set_view(V_PART);
    key(white, 1);
    tick(2);
    smk_clip_enter_cleanup();                         /* the hardware clip-entry hook */
    tick(20);
    CHECK(!(engine_midi_held() & 1), "clip entry closes the held step preview");
    key(white, 0);
    CHECK(a->flags == G_NOTE, "clip entry does not turn the old step off");

    key(white, 1);
    tick(2);
    btn(B_REC, 1);                                  /* REC changes the same keybed to keyboard notes */
    btn(B_REC, 0);
    tick(20);
    CHECK(ui.rec && !(engine_midi_held() & 1), "REC closes the old step preview");
    key(white, 0);
    CHECK(a->flags == G_NOTE, "REC transition does not toggle the old step");
    btn(B_REC, 1);
    btn(B_REC, 0);

    key(white, 1);
    tick(2);
    seq.ppat[PART_303A] = 1;                        /* a queued pattern can change during a hold */
    tick(20);
    CHECK(!(engine_midi_held() & 1), "pattern change closes the old step preview");
    key(white, 0);
    CHECK(a->flags == G_NOTE, "pattern change leaves the original step on");
    CHECK(bstep_gate(&proj.pat[1].bass[0].step[0]) == G_REST,
          "old key-up does not edit the newly selected pattern");
    seq.ppat[PART_303A] = 0;
    tick(2);

    tap_key(white, 20);
    CHECK(bstep_gate(a) == G_REST, "ordinary tap still turns an existing step off");
    tap_key(white, 20);
    CHECK(bstep_gate(a) == G_NOTE, "ordinary tap still creates an empty step");
    key(white, 1);
    tick(350);
    key(white, 0);
    CHECK(bstep_gate(a) == G_NOTE, "ordinary hold still keeps a step on");
    printf("  303 step preview: part, view and clip transitions release sound; taps keep their meaning: checked\n");
}

static void test_303_auto_slide(void)
{
    bpart_t *bp;
    seq_stop(&seq);
    ui.part = PART_303A;
    set_view(V_PART);
    ui.rec = 0;
    ui.freeplay[0] = 0;
    build_pages();
    bp = &cur_pat()->bass[0];
    bp->len = 16;
    memset(bp->step, 0, sizeof bp->step);
    btn(B_REC, 1); btn(B_REC, 0);
    key(10, 1);
    key(12, 1);
    CHECK(bst(0)->flags == (G_NOTE | BS_SLIDE), "step REC overlap marks source slide");
    CHECK(bst(1)->flags == G_NOTE, "step REC destination has no outgoing slide");
    key(10, 0); key(12, 0);
    key(14, 1); key(14, 0);
    CHECK(bst(1)->flags == G_NOTE, "released key does not create a slide");
    ui.wpos[0] = 15;
    key(10, 1); key(12, 1);
    CHECK(bst(15)->flags & BS_SLIDE, "step REC slide wraps to step zero");
    key(10, 0); key(12, 0);
    ui.wpos[0] = 3;
    key(10, 1);
    bass_write(0, 0, G_REST);
    key(12, 1);
    CHECK(!(bst(3)->flags & BS_SLIDE), "explicit rest breaks automatic slide gesture");
    key(10, 0); key(12, 0);
    ui.wpos[0] = 6;
    key(10, 1);
    btn(B_REC, 1); btn(B_REC, 0);
    btn(B_REC, 1); btn(B_REC, 0);
    key(12, 1);
    CHECK(!(bst(6)->flags & BS_SLIDE), "REC restart cannot slide from stale source");
    key(10, 0); key(12, 0);

    ui.wpos[0] = 7;
    key(10, 1);
    ui.part = PART_303B;
    ui.wpos[1] = 0;
    memset(cur_pat()->bass[1].step, 0, sizeof cur_pat()->bass[1].step);
    key(12, 1);
    CHECK(!(bst(7)->flags & BS_SLIDE), "part change cannot alter the old part's slide");
    key(14, 1);
    CHECK(cur_pat()->bass[1].step[0].flags & BS_SLIDE, "303 B also records automatic slides");
    key(10, 0); key(12, 0); key(14, 0);
    ui.part = PART_303A;
    ui.wpos[0] = 9;
    key(10, 1);
    {
        int oldpat = seq.ppat[PART_303A];
        seq.ppat[PART_303A] = (uint8_t)((oldpat + 1) % NPAT);
        key(12, 1);
        CHECK(!(proj.pat[oldpat].bass[0].step[9].flags & BS_SLIDE),
              "pattern change cannot link back to the old pattern");
        key(10, 0); key(12, 0);
        seq.ppat[PART_303A] = (uint8_t)oldpat;
    }
    memset(bp->step, 0, sizeof bp->step);
    seq.bpm = 120.0f;
    btn(B_PLAY, 1); btn(B_PLAY, 0);
    at_step(TRK_BASS0, 4, 10);
    btn(B_ENV, 1);
    key(10, 1);
    btn(B_ENV, 0);
    at_step(TRK_BASS0, 5, 10);
    key(12, 1);
    CHECK((bst(4)->flags & 15) == (G_NOTE | BS_ACCENT | BS_SLIDE), "live overlap slides source and retains accent (flags=%u recactive=%u last=%u mask=%u)", bst(4)->flags, seq.rec_bass[0].active,seq.rec_bass[0].last,seq.rec_mask);
    CHECK((bst(5)->flags & 15) == G_NOTE, "live destination has no outgoing slide (flags=%u)",bst(5)->flags);
    key(10, 0); key(12, 0);
    at_step(TRK_BASS0, 6, 10);
    key(14, 1); key(14, 0);
    CHECK(!(bst(5)->flags & BS_SLIDE), "live detached notes do not slide");
    at_step(TRK_BASS0, 8, 10);
    key(10, 1); key(12, 1);
    CHECK((bst(8)->flags & 15) == G_NOTE && bst(8)->note == kbd_note(12),
          "two live notes on one quantized step replace pitch without a self-slide (flags=%u note=%u)",bst(8)->flags,bst(8)->note);
    key(10, 0); key(12, 0);
    at_step(TRK_BASS0, 15, 10);
    key(10, 1);
    at_step(TRK_BASS0, 0, 10);
    key(12, 1);
    CHECK(bst(15)->flags & BS_SLIDE, "live overlap slides across loop boundary");
    key(10, 0); key(12, 0);
    btn(B_REC, 1); btn(B_REC, 0);
    btn(B_PLAY, 1); btn(B_PLAY, 0);
    tick(20);
    printf("  303 automatic REC slides: overlap, release, wrap, same step, accent, rest and REC restart checked\n");
}

static void test_303_live(void)
{
    int s, n_lfo = kbd_note(15), n_env = kbd_note(14), n_plain = kbd_note(22), n_after = kbd_note(10);
    ui.part = PART_303A;
    set_view(V_PART);
    build_pages();
    proj.pat[seq.ppat[PART_303A]].bass[0].len = 16;
    for (s = 0; s < 16; s++) {
        bst(s)->note = 30;
        bst(s)->flags = G_NOTE;
    }
    tick(700);
    seq.bpm = 120.0f;
    btn(B_REC, 1);
    btn(B_REC, 0);
    btn(B_PLAY, 1);
    btn(B_PLAY, 0);
    tick(20);
    CHECK(ui.rec && seq.playing, "REC on and playing (rec %d playing %d)", ui.rec, seq.playing);
    at_step(TRK_BASS0, 0, 0);
    /* LFO + key on step 11 */
    at_step(TRK_BASS0, 10, 10);
    btn(B_LFO, 1);
    tap_key(15, 30);
    btn(B_LFO, 0);
    tick(5);
    CHECK(bst(10)->note == n_lfo && bstep_gate(bst(10)) == G_NOTE, "LFO + key: step 11 note %d, want %d", bst(10)->note, n_lfo);
    CHECK(bst(10)->flags & BS_SLIDE, "LFO + key: step 11 has no slide (flags 0x%02x)", bst(10)->flags);
    CHECK(!(bst(10)->flags & BS_ACCENT), "LFO + key: step 11 has an accent (flags 0x%02x)", bst(10)->flags);
    CHECK(ui.view == V_PART, "LFO released after LFO + key: view %d, want V_PART %d (V_MIX %d)", ui.view, V_PART, V_MIX);
    /* a plain key on step 14: it records, without a slide */
    at_step(TRK_BASS0, 13, 10);
    tap_key(22, 30);
    tick(5);
    CHECK(bst(13)->note == n_plain && !(bst(13)->flags & (BS_SLIDE | BS_ACCENT)),
          "plain key after LFO + key: step 14 note %d flags 0x%02x, want note %d, no slide", bst(13)->note, bst(13)->flags, n_plain);
    /* ENV + key on step 3 (next bar): an accent; ENV released: no FX view */
    at_step(TRK_BASS0, 2, 10);
    btn(B_ENV, 1);
    tap_key(14, 30);
    btn(B_ENV, 0);
    tick(5);
    CHECK(bst(2)->note == n_env && (bst(2)->flags & BS_ACCENT) && !(bst(2)->flags & BS_SLIDE),
          "ENV + key: step 3 note %d flags 0x%02x, want note %d, accent", bst(2)->note, bst(2)->flags, n_env);
    CHECK(ui.view == V_PART, "ENV released after ENV + key: view %d, want V_PART %d", ui.view, V_PART);
    at_step(TRK_BASS0, 5, 10);
    tap_key(10, 30);
    tick(5);
    CHECK(bst(5)->note == n_after, "plain key after ENV + key: step 6 note %d, want %d", bst(5)->note, n_after);
    CHECK(bst(0)->note == 30 && bst(0)->flags == G_NOTE, "untouched step 1 changed: note %d flags 0x%02x", bst(0)->note, bst(0)->flags);
    btn(B_REC, 1);
    btn(B_REC, 0);
    btn(B_PLAY, 1);
    btn(B_PLAY, 0);
    tick(300);
    printf("  303 live record: LFO + key slides, ENV + key accents, the keys record after the release: checked\n");
}

static void clear_909(void)
{
    memset(proj.pat[seq.ppat[0]].drum[0].hit, 0, sizeof proj.pat[0].drum[0].hit);
    memset(proj.pat[seq.ppat[0]].drum[0].accent, 0, sizeof proj.pat[0].drum[0].accent);
}

static void test_start_ext_running(void)
{
    int s, n, stop_at, run_no;
    ui.part = PART_909;
    set_view(V_PART);
    build_pages();
    clear_909();
    tick(1500);
    btn(B_REC, 1);
    btn(B_REC, 0);
    CHECK(ui.rec && !seq.playing, "REC on, stopped");
    clk_on = 1;                                       /* 120 BPM, 7.5 ms connection events */
    clk_tick_ms = 60000.0 / 120.0 / 24.0;
    clk_ev_ms = 7.5;
    clk_next_tick = clk_next_ev = now_ms + 1;
    tick(500);
    CHECK(seq.ext && !seq.playing, "the clock runs, X0X stopped (ext %d playing %d)", seq.ext, seq.playing);
    for (run_no = 0; run_no < 3; run_no++) {
        midi3(0xFA, 0, 0);                            /* Start, play to step 6 + 2 * run_no + 1, Stop */
        tick(10);
        CHECK(seq.playing, "Start: playing");
        at_step(TRK_DRUM, 6 + 2 * run_no, 40);
        midi3(0xFC, 0, 0);
        tick(30);
        stop_at = eng_step[TRK_DRUM];
        CHECK(!seq.playing && stop_at != 0, "Stop: stopped at step %d", stop_at + 1);
        clear_909();
        tick(300);
        {   /* a tick, then Start 2 ms later, then a hit before the next tick */
            uint32_t c = clk_sent;
            while (clk_sent == c)
                tick(1);
            while (mi_r != mi_w)
                tick(1);
            tick(2);
            midi3(0xFA, 0, 0);
            n = 0;
            while (!seq.playing && n++ < 30)
                tick(1);
            CHECK(seq.playing && !seq.t[TRK_DRUM].started, "Start: playing %d, first step played %d (the test needs 1, 0)",
                  seq.playing, seq.t[TRK_DRUM].started);
            key(BLACK_K[9], 1);
            tick(3);
            key(BLACK_K[9], 0);
        }
        tick(60);
        for (s = 0, n = 0; s < 64; s++)
            n += sm_get(proj.pat[seq.ppat[0]].drum[0].hit[9], s);
        CHECK(sm_get(proj.pat[seq.ppat[0]].drum[0].hit[9], 0) && n == 1,
              "run %d: the hit before the first tick after Start: step 1 %d, steps set %d, the old run stopped at step %d (set %d)",
              run_no, sm_get(proj.pat[seq.ppat[0]].drum[0].hit[9], 0), n, stop_at + 1,
              sm_get(proj.pat[seq.ppat[0]].drum[0].hit[9], stop_at));
        midi3(0xFC, 0, 0);
        tick(30);
    }
    clk_on = 0;
    btn(B_REC, 1);
    btn(B_REC, 0);
    tick(800);
    printf("  Start with the clock running, a hit before the first tick: step 1: checked\n");
}

/* a73 (owner OK 2026-10-16 20:11): a live drum hit records on the nearest step */
static void test_drum_nearest(void)
{
    static const struct { int after, ms, v, want; } H[] = {
        {3, 105, 2, 4},                               /* about 20 ms before step 5: step 5 */
        {8, 20, 3, 8},                                /* 20 ms after step 9: step 9 */
        {11, 50, 4, 11},                              /* 50 ms after step 12 (half: 62.5 ms): step 12 */
        {12, 80, 5, 13},                              /* about 45 ms before step 14: step 14 */
        {15, 105, 6, 0},                              /* about 20 ms before the loop start: step 1 */
    };
    int i, s, n;
    ui.part = PART_909;
    set_view(V_PART);
    build_pages();
    clear_909();
    tick(700);
    seq.bpm = 120.0f;
    btn(B_REC, 1);
    btn(B_REC, 0);
    btn(B_PLAY, 1);
    btn(B_PLAY, 0);
    tick(20);
    CHECK(ui.rec && seq.playing, "REC on and playing");
    at_step(TRK_DRUM, 0, 0);
    for (i = 0; i < 5; i++) {
        at_step(TRK_DRUM, H[i].after, (uint32_t)H[i].ms);
        tap_key(BLACK_K[H[i].v], 10);
    }
    tick(50);
    for (i = 0; i < 5; i++) {
        for (s = 0, n = 0; s < 64; s++)
            n += sm_get(proj.pat[seq.ppat[0]].drum[0].hit[H[i].v], s);
        CHECK(n == 1 && sm_get(proj.pat[seq.ppat[0]].drum[0].hit[H[i].v], H[i].want),
              "hit %d ms after step %d: step %d set %d, steps set %d", H[i].ms, H[i].after + 1, H[i].want + 1,
              sm_get(proj.pat[seq.ppat[0]].drum[0].hit[H[i].v], H[i].want), n);
    }
    btn(B_REC, 1);
    btn(B_REC, 0);
    btn(B_PLAY, 1);
    btn(B_PLAY, 0);
    tick(300);
    printf("  909 live record: the nearest step, step 1 for a hit just before the loop start: checked\n");
}

static void test_filter_controls(void)
{
    const int params[] = {BASS303_FATTACK, BASS303_DECAY, BASS303_FSUSTAIN, BASS303_FRELEASE, BASS303_FENV};
    const int values[] = {32, 80, 64, 90, 1};
    int i, part;
    seq_stop(&seq);
    ui.rec = 0;
    tick(20);
    for (part = 0; part < NBASS; part++)
        for (i = 0; i < 5; i++)
            pref_set(PR(R_ENG, T_303, part, params[i]), values[i]);
    CHECK(project_save() == 0, "filter ADSR sound saves in existing project format");
    memset(&proj.sound, 0, sizeof proj.sound);
    CHECK(project_load() == 0, "filter ADSR sound reloads");
    for (part = 0; part < NBASS; part++)
        for (i = 0; i < 5; i++)
            CHECK(proj.sound.v[T_303][part][params[i]] == values[i], "filter ADSR control survives reload");
    ui.part = PART_303A;
    set_view(V_PART);
    build_pages();
    *page_of(V_PART) = 2;
    tick(50);
    png_write("build/host/filter-adsr.png");
}


/* Timestamped production MIDI path: USB event queue -> engine render -> recorder.
 * Playback below uses the production sequencer and its real gate/slide sink API. */
static unsigned capture_clock;
static double capture_energy;
static void capture_frames(unsigned n)
{
    int32_t out[512];
    while (n) {
        unsigned k = n > 256 ? 256 : n;
        engine_render(out, k);
        for (unsigned i = 0; i < k * 2; i++) {
            double v = out[i] / 8388608.0;
            CHECK(isfinite(v), "record audio finite");
            capture_energy += v * v;
        }
        capture_clock += k;
        n -= k;
    }
    input();
}
static void capture_midi(unsigned at, unsigned src, int note, int velocity)
{
    CHECK(at >= capture_clock, "ordered timestamp");
    capture_frames(at - capture_clock);
    uint32_t st = (velocity ? 0x90u : 0x80u) | seq.ch[TRK_BASS0];
    min_q[mi_w++ % MQ] = (src << 4) | (st >> 4) | (st << 8) |
                        ((uint32_t)note << 16) | ((uint32_t)velocity << 24);
    capture_frames(1); /* actual production MIDI queue consumption */
}
static void capture_reset(int len, int swing)
{
    seq_stop(&seq); capture_frames(1);
    ui.part = PART_303A; ui.rec = 1; ui.freeplay[0] = 1;
    set_view(V_PART);
    for (int p = 0; p < NPAT; p++) pattern_init(&proj.pat[p], 1, 2);
    proj.pat[0].bass[0].len = (uint8_t)len;
    proj.pat[0].swing = (uint8_t)swing;
    seq_init(&seq, proj.pat); seq.bpm = 120; seq.clk_src = SEQ_CLK_INT;
    seq.send_notes = seq.send_clock = 0;
    input(); seq_start(&seq); capture_frames(1);
    capture_clock = 0; capture_energy = 0;
}
static void capture_rt(unsigned at, int st)
{
    capture_frames(at - capture_clock);
    min_q[mi_w++ % MQ] = 0x0Fu | ((uint32_t)st << 8);
    capture_frames(1);
}
typedef struct { unsigned at; int kind, note, slide; } capture_event_t;
static capture_event_t capture_events[128];
static unsigned capture_nevents, playback_clock;
static int capture_on(void *ctx, int b, int note, int accent, int slide, int velocity)
{
    (void)ctx; (void)accent; (void)velocity;
    if (!b && capture_nevents < 128)
        capture_events[capture_nevents++] = (capture_event_t){playback_clock, 1, note, slide};
    return 1;
}
static void capture_off(void *ctx, int b)
{
    (void)ctx;
    if (!b && capture_nevents < 128)
        capture_events[capture_nevents++] = (capture_event_t){playback_clock, 0, 0, 0};
}
static void capture_playback(unsigned n)
{
    seq_t playback;
    seq_sink_t sink = {0}; sink.bass_on = capture_on; sink.bass_off = capture_off;
    /* Empty drums still invoke drum only for hits; clear both here. */
    for (int k = 0; k < NKIT; k++) memset(proj.pat[0].drum[k].hit, 0, sizeof proj.pat[0].drum[k].hit);
    seq_init(&playback, proj.pat); playback.bpm = 120; playback.clk_src = SEQ_CLK_INT;
    playback.send_clock = playback.send_notes = 0;
    capture_nevents = playback_clock = 0;
    seq_start(&playback); seq_advance(&playback, 0, &sink);
    while (playback_clock < n) {
        unsigned k = seq_until_event(&playback, n - playback_clock);
        playback_clock += k; seq_advance(&playback, k, &sink);
    }
}
static unsigned capture_first_off(void)
{
    int seen = 0;
    for (unsigned i = 0; i < capture_nevents; i++) {
        if (capture_events[i].kind) seen = 1;
        else if (seen) return capture_events[i].at;
    }
    return ~0u;
}
static void test_timestamped_live_duration(void)
{
    bpart_t *bp = &proj.pat[0].bass[0];
    capture_reset(16, 0);
    capture_midi(441, 0, 48, 90); /* press 10 ms after step zero */
    capture_frames(14000);
    CHECK(engine_midi_held() & 1, "live MIDI remains gated beyond multiple legacy half-step cutoffs");
    CHECK(capture_energy > 0, "held MIDI renders nonzero continuous audio without reinitializing voice");
    capture_midi(15435, 0, 48, 0); /* actual release at 350 ms */
    CHECK(!(engine_midi_held() & 1), "actual MIDI release ends live voice");
    CHECK(bstep_gate(&bp->step[0]) == G_NOTE && !(bp->step[0].flags >> 4), "onset plus continuation has no premature endpoint");
    CHECK(bstep_gate(&bp->step[1]) == G_TIE && !(bp->step[1].flags >> 4), "middle tie sustains whole cell");
    CHECK(bstep_gate(&bp->step[2]) == G_TIE && (bp->step[2].flags >> 4) == 12, "release captured at 12/15 terminal cell");
    CHECK(ui.dirty, "production MIDI recorder marks UI dirty");
    capture_playback(20000);
    CHECK(abs((int)capture_first_off() - 15435) <= 185, "playback gate ends at recorded release within half endpoint bin (off=%u)", capture_first_off());
    unsigned ons = 0; for (unsigned i = 0; i < capture_nevents; i++) ons += capture_events[i].kind;
    CHECK(ons == 1, "held NOTE/TIE replay has one onset, no tail retrigger");

    capture_reset(16, 0);
    capture_midi(441, 0, 48, 90);
    capture_midi(6000, 0, 55, 90); /* overlap -> outgoing slide on previous NOTE */
    capture_midi(6200, 0, 48, 0); /* old release cannot truncate newest */
    CHECK(engine_midi_held() & 1, "older note release preserves active last-press gate");
    capture_midi(7000, 1, 55, 0); /* foreign route same note must not release */
    CHECK(engine_midi_held() & 1, "foreign route same-pitch release isolated");
    capture_midi(10000, 0, 55, 0);
    CHECK(bp->step[0].flags & BS_SLIDE, "overlap stores source slide");
    capture_playback(15000);
    CHECK(capture_nevents >= 3 && capture_events[1].kind && capture_events[1].note == 55 && capture_events[1].slide,
          "production playback overlaps destination as slide without intermediate off");

    capture_reset(16, 0);
    capture_midi(441, 0, 48, 90); capture_midi(3000, 0, 48, 0);
    capture_midi(6000, 0, 48, 90); capture_midi(9000, 0, 48, 0);
    CHECK(!(bp->step[0].flags & BS_SLIDE), "released/repeated pitch does not create legato");
    capture_playback(13000);
    ons = 0; for (unsigned i = 0; i < capture_nevents; i++) if (capture_events[i].kind) {
        ons++; CHECK(!capture_events[i].slide, "released repeated pitch replay retriggers");
    }
    CHECK(ons == 2, "released same-pitch events produce two onsets");

    capture_reset(16, 0);
    capture_midi(441, 0, 0, 90); capture_midi(6000, 0, 0, 90);
    capture_midi(6500, 0, 0, 0);
    CHECK(engine_midi_held() & 1, "MIDI note zero and repeated press count survive first off");
    capture_midi(9000, 0, 0, 0);
    CHECK(!(engine_midi_held() & 1), "repeated note zero releases on last matching off");
    CHECK(bstep_gate(&bp->step[0]) == G_NOTE && bstep_gate(&bp->step[1]) == G_NOTE && !(bp->step[0].flags & BS_SLIDE),
          "overlapping same pitch stores separate retrigger events");

    capture_reset(4, 0);
    capture_midi(17000, 0, 48, 90); /* step three -> next loop */
    capture_midi(28000, 0, 48, 0);
    CHECK(bstep_gate(&bp->step[3]) == G_NOTE && bstep_gate(&bp->step[0]) == G_TIE && bstep_gate(&bp->step[1]) == G_TIE,
          "long hold records across loop boundary with preserved onset");
    /* Starting replay before an orphan continuation intentionally waits for NOTE. */
    capture_playback(44000);
    CHECK(capture_first_off() > 27000 && capture_first_off() < 28500, "wrapped gate replay releases at endpoint");

    capture_reset(16, 100);
    capture_midi(441, 0, 48, 90); capture_midi(6000, 0, 48, 0);
    capture_playback(10000);
    CHECK(abs((int)capture_first_off() - 6000) <= 280, "swing endpoint uses actual slot length (off=%u)", capture_first_off());

    capture_reset(16, 0);
    capture_midi(441, 0, 48, 90); capture_frames(6000);
    ui.part = PART_303B; input(); capture_frames(1);
    CHECK(!seq.rec_bass[0].active, "selection change finalizes original destination");
    capture_midi(9000, 0, 48, 0);
    CHECK(bstep_gate(&proj.pat[0].bass[1].step[0]) == G_REST, "release after selection does not write new part");
    capture_reset(16, 0);
    capture_midi(441, 0, 48, 90); capture_frames(6000);
    seq_stop(&seq); capture_frames(1);
    CHECK(!seq.rec_bass[0].active && !(engine_midi_held() & 1), "stop while held finalizes recording and live gate");
    capture_midi(9000, 0, 48, 0);


    capture_reset(16, 0);
    seq_stop(&seq); capture_frames(1); seq.clk_src = SEQ_CLK_EXT; capture_clock = 0;
    capture_rt(0, 0xFA); capture_rt(100, 0xF8);
    capture_midi(200, 0, 48, 90);
    for (unsigned i = 1; i < 12; i++) capture_rt(100 + i * 919 + (i % 2 ? 20 : 0), 0xF8);
    capture_midi(11025, 0, 48, 0);
    capture_playback(15000);
    CHECK(abs((int)capture_first_off() - 10925) <= 370,
          "external jittered clocks preserve note-off within endpoint grid and observed clock jitter (off=%u)", capture_first_off());
    capture_reset(16, 0);
    seq_stop(&seq); capture_frames(1); seq.clk_src = SEQ_CLK_EXT; capture_clock = 0;
    capture_rt(0, 0xFA); capture_rt(100, 0xF8);
    capture_frames(4000 - capture_clock);
    for (int i = 0; i < 4; i++) min_q[mi_w++ % MQ] = 0x0Fu | (0xF8u << 8);
    min_q[mi_w++ % MQ] = 0x09u | ((uint32_t)(0x90 | seq.ch[TRK_BASS0]) << 8) | (48u << 16) | (90u << 24);
    capture_frames(1);
    CHECK(seq.rec_bass[0].first == 1, "clocks before note in one drain update authoritative quantization position");
    capture_midi(4500, 0, 48, 0);

    /* Physical keyboard live REC keeps the established upcoming-step MIDI echo. */
    capture_reset(16, 0); seq.send_notes = 1;
    capture_frames(3500);
    unsigned prior_ons = note_ons;
    engine_bass_on_velocity(0, 48, 0, 0, 90);
    engine_bass_record(0, 48, 90, 1, 0);
    capture_frames(2300);
    CHECK(note_ons == prior_ons + 1 && seq.bass_out_valid[0], "physical recording echoes snapped future onset once without internal retrigger");
    engine_bass_off(0); engine_bass_record(0, 48, 0, 0, 0); capture_frames(1);
    CHECK(!seq.bass_out_valid[0], "physical release flushes sequencer MIDI gate");


    capture_reset(16, 0);
    capture_midi(441, 0, 48, 90); capture_frames(6000);
    ui.rec = 0; input(); capture_frames(1);
    CHECK(!seq.rec_bass[0].active && !(engine_midi_held() & 1), "REC off finalizes original capture and releases voice");
    ui.rec = 1; input();
    capture_midi(7000, 0, 48, 90);
    capture_midi(7500, 0, 48, 0); /* old membership off, after new attack */
    CHECK(engine_midi_held() & 1, "stale old same-pitch off after context end cannot cut new press");
    capture_midi(9000, 0, 48, 0);
    CHECK(!(engine_midi_held() & 1), "new same-pitch press ends on its final off");
    capture_reset(16, 0);
    capture_midi(441, 0, 48, 90); capture_frames(6000);
    run_action(ACT_CLEAR_PART, PART_303A); input();
    capture_midi(7000, 0, 55, 90); /* new press shares drain with pending context off */
    CHECK(engine_midi_held() & 1, "pending context gate-off is consumed before successor press");
    capture_midi(7500, 0, 48, 0);
    CHECK(engine_midi_held() & 1, "old different-pitch off cannot cut successor after clear");
    capture_midi(9000, 0, 55, 0);
    /* Exact release boundary and its adjacent samples share one nearest bin. */
    for (int jitter = -1; jitter <= 1; jitter++) {
        capture_reset(16, 0);
        capture_midi(441, 0, 48, 90);
        capture_midi((unsigned)(11025 + jitter), 0, 48, 0);
        capture_playback(14000);
        CHECK(abs((int)capture_first_off() - 11025) <= 1, "boundary jitter %d resolves to exact prior-cell endpoint (off=%u)", jitter, capture_first_off());
    }
    capture_reset(4, 0);
    capture_midi(441, 0, 48, 90); capture_midi(60000, 0, 48, 0);
    CHECK(bstep_gate(&bp->step[0]) == G_NOTE && (bp->step[3].flags >> 4) == 15,
          "hold beyond full loop saturates one traversal without deleting onset");
    capture_playback(23000);
    CHECK(capture_first_off() >= 22049 && capture_first_off() <= 22051, "saturated periodic pattern ends after one full loop");

    capture_reset(4, 0);
    bp->dir = DIR_REV;
    seq_stop(&seq); capture_frames(1); seq_start(&seq); capture_frames(1); capture_clock = 0;
    capture_midi(441, 0, 48, 90); capture_midi(11025, 0, 48, 0);
    CHECK(bstep_gate(&bp->step[3]) == G_NOTE && bstep_gate(&bp->step[2]) == G_TIE,
          "reverse recording fills actual traversal");
    capture_playback(15000);
    CHECK(abs((int)capture_first_off() - 11025) <= 1, "reverse boundary has no orphan continuation");

    capture_reset(16, 0); undo_start();
    capture_midi(441, 0, 48, 90); capture_midi(9000, 0, 48, 0);
    bpart_t recorded = *bp;
    do_undo(0);
    CHECK(bstep_gate(&bp->step[0]) == G_REST, "actual undo restores pre-recording line");
    do_undo(1);
    CHECK(!memcmp(bp, &recorded, sizeof recorded), "actual redo preserves all captured endpoint bytes");
    for (int i = 0; i < NPARTS; i++) ui.paste_src[i] = 0;
    run_action(ACT_COPY, 1);
    CHECK(!memcmp(&proj.pat[1].bass[0], &recorded, sizeof recorded), "UI pattern copy preserves captured duration");
    seq_stop(&seq); capture_frames(1); ui.rec = 0;
    CHECK(project_save() == 0, "captured endpoint project saves");
    memset(bp->step, 0, sizeof bp->step);
    CHECK(project_load() == 0 && !memcmp(bp, &recorded, sizeof recorded), "current stream4 save/load preserves exact capture bytes");


    CHECK(get32(store[OBJ_PAT0]) == 4, "new save names endpoint format4");
    /* Valid format3 has the identical layout but no endpoint bits. Rewrite only
     * format words on a newly saved all-legacy line to exercise actual loader. */
    for (int p = 0; p < NPAT; p++) for (int b = 0; b < NBASS; b++)
        for (int i = 0; i < NSTEPS; i++) proj.pat[p].bass[b].step[i].flags &= 15u;
    CHECK(project_save() == 0, "legacy-shaped stream generated");
    for (int i = 0; i < NSTREAM; i++) put32(store[STREAM_OBJ[i]], 3);
    memset(bp->step, 0, sizeof bp->step);
    CHECK(project_load() == 0 && bp->step[0].flags == G_NOTE && bp->step[0].velocity == 90,
          "actual legacy format3 loads unchanged with zero endpoint");

    capture_reset(16, 0);
    engine_bass_record(0, 48, 90, 1, 0); /* admitted edge, not drained yet */
    run_action(ACT_CLEAR_PART, PART_303A);
    input(); capture_frames(1);
    CHECK(!seq.rec_bass[0].active && bstep_gate(&bp->step[0]) == G_REST,
          "bulk edit invalidates pending keyboard recording epoch before re-arm");
    ui.rec = 0;
    puts("  timestamped MIDI recording: duration, gate replay, slide, retrigger, route isolation, note zero, wrap, swing, selection and stop checked");
}

int main(void)
{
    boot();
    tick(300);
    test_303_freeplay_velocity();
    test_303_key_release();
    test_303_step_preview_lifecycle();
    test_303_auto_slide();
    test_303_live();
    test_start_ext_running();
    test_drum_nearest();
#if BOARD_SMK37
    seq_stop(&seq);
    tick(20);
    ui.rec = 0;
    test_clip_mode = test_clip_busy = 1;
    ui.act_t = plat_ms();
    save_project();
    CHECK(save_wait, "manual SAVE waits while a clip key is held");
    test_clip_busy = 0;
    autosave();
    CHECK(save_wait, "queued clip SAVE still waits for quiet time");
    ui.act_t = plat_ms() - AUTOSAVE_QUIET;
    autosave();
    CHECK(!save_wait, "queued clip SAVE runs after clip input is quiet");
    seq.playing = 1;
    CHECK(!strcmp(queued_save_label(), "SAVE AT STOP"), "playing clip-mode save names the stop condition");
    seq.playing = 0;
    CHECK(!strcmp(queued_save_label(), "SAVE WHEN QUIET"), "stopped clip-mode save names the quiet condition");
    test_clip_mode = 0;
#endif
    test_filter_controls();
    test_timestamped_live_duration();
    printf("rec_live: %s\n", fails ? "FAIL" : "ok");
    return fails != 0;
}
