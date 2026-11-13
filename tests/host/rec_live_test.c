/* SPDX-License-Identifier: GPL-3.0-only */
/* Live recording on the whole app (the host simulator, host/x0x_host.c without its script main):
 *  - 303: ENV or LFO held with a key writes an accent or a slide, as step input does. The release of
 *    ENV or LFO does not open the FX or MIX view, and the next keys still record (a73).
 *  - 909: after a Start with the external clock already running, a hit before the first tick goes to
 *    step 1, not to the step of the last run (a73).
 *  - 909: a live hit records on the nearest step; just before the loop start: step 1 (a73).
 * Build: see tests/run_tests.sh (needs build/gen from host/build_host.sh). */
#define main x0x_host_main
#include "../../host/x0x_host.c"
#undef main

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
    tick(1);
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

int main(void)
{
    boot();
    tick(300);
    test_303_live();
    test_start_ext_running();
    test_drum_nearest();
    printf("rec_live: %s\n", fails ? "FAIL" : "ok");
    return fails != 0;
}
