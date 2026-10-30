/* SPDX-License-Identifier: GPL-3.0-only */
/* External MIDI recording for 909, 808 and BREAK (the whole app on the host simulator):
 *  - REC on, playing, the part selected in SEQ: a Note On on the part's own channel writes a step
 *    (drums: the GM_DRUM voice with its velocity in T_VEL; BREAK: the step on, with that slice).
 *  - a note on another channel writes nothing; with REC off nothing is written.
 *  - owner 2026-10-30: REC on and playing, EVERY part (909, 808, BREAK, Chicago/Miami A and B)
 *    records its own channel at the same time, whatever part or view is selected.
 * Build: tests/host/run_ext_midi_rec.sh (SMK-37). */
#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1 /* expose host libproc instrumentation under POSIX feature selection */
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
/* This fixture models disconnected SMC; project_smc_runtime_test.c covers
 * the production SMC project-active/reset implementation. */
/* This link does not include smc_mixer.c. ui.c still asks whether an SMC solo holds the project. */
int smc_project_active(void) { return 0; }
void smc_project_reset(void) {}
void smc_frame(void) {}
void smc_poll(void) {}
int smc_midi_host(uint32_t pkt) { (void)pkt; return 0; }
/* Presentation lives in smc_mixer.c; this fixture is disconnected SMC / live-rec only. */
void smc_ui_cancel(void) {}
int smc_ui_active(void) { return 0; }
void smc_ui_draw(void) {}
void smc_ui_clip_show(int n, const char *name, const char *value)
{ (void)n; (void)name; (void)value; }
#endif

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("  FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static void midi3(int s, int a, int b)
{
    min_q[mi_w++ % MQ] = (uint32_t)s >> 4 | (uint32_t)s << 8 | (uint32_t)a << 16 | (uint32_t)b << 24;
}
static void tick(uint32_t ms) { run_ms(ms); }
static void btn(int b, int down)
{
    if (down) held_btn |= 1u << b; else held_btn &= ~(1u << b);
    tick(1);
}
static void press(int b) { btn(b, 1); btn(b, 0); }
static void set_rec(int on) { if (!!ui.rec != !!on) press(B_REC); }
static void set_play(int on) { if (!!seq.playing != !!on) press(B_PLAY); tick(20); }

static int free_channel(void)
{
    for (int c = 15; c >= 0; c--) {
        int used = 0;
        for (int t = 0; t < NTRACKS; t++) used |= seq.ch[t] == c;
        if (!used) return c;
    }
    return -1;
}
static int drum_hits(int k, int v)
{
    int n = 0;
    for (int s = 0; s < NSTEPS; s++) n += sm_get(proj.pat[seq.ppat[k]].drum[k].hit[v], s);
    return n;
}
static int drum_first(int k, int v)
{
    for (int s = 0; s < NSTEPS; s++) if (sm_get(proj.pat[seq.ppat[k]].drum[k].hit[v], s)) return s;
    return -1;
}
static void clear_drum(int k)
{
    memset(proj.pat[seq.ppat[k]].drum[k].hit, 0, sizeof proj.pat[0].drum[0].hit);
    memset(proj.pat[seq.ppat[k]].drum[k].accent, 0, sizeof proj.pat[0].drum[0].accent);
}
static void select_part(int part)
{
    set_play(0);
    set_rec(0);
    ui.part = (uint8_t)part;
    set_view(V_SEQ);
    build_pages();
    tick(300);
}

static void test_drum(int k, const char *name)
{
    int v = 2, note = GM_DRUM[k][v], ch = seq.ch[TRK_DRUM + k], other = free_channel(), s, lane;
    select_part(k);
    clear_drum(k);
    CHECK(other >= 0, "%s: a free MIDI channel for the wrong-channel case", name);
    set_rec(1);
    set_play(1);
    CHECK(ui.rec && seq.playing && ui.view == V_SEQ, "%s: REC on, playing, SEQ", name);
    tick(120);
    midi3(0x90 | ch, note, 77); tick(5); midi3(0x80 | ch, note, 0); tick(20);
    CHECK(drum_hits(k, v) == 1, "%s: own-channel note writes one step (steps %d)", name, drum_hits(k, v));
    s = drum_first(k, v);
    lane = motion_find(proj.arr.lane, seq.ppat[k], k, T_VEL, k << 4 | v, 0);
    CHECK(s >= 0 && lane >= 0 && proj.arr.lane[lane].val[s] == 77, "%s: MIDI velocity 77 in T_VEL (got %d)", name,
          s >= 0 && lane >= 0 ? proj.arr.lane[lane].val[s] : -1);
    clear_drum(k);
    midi3(0x90 | other, note, 77); tick(5); midi3(0x80 | other, note, 0); tick(20);
    CHECK(drum_hits(k, v) == 0, "%s: other-channel note writes nothing", name);
    set_rec(0);
    tick(5);
    midi3(0x90 | ch, note, 77); tick(5); midi3(0x80 | ch, note, 0); tick(20);
    CHECK(drum_hits(k, v) == 0, "%s: REC off: nothing written", name);
    set_play(0);
    printf("  %s external MIDI record: own channel writes, other channel and REC off do not: checked\n", name);
}

static void test_break(void)
{
    brkpart_t *bp;
    int ch = seq.ch[TRK_BRK], other = free_channel(), slice = 2, note = 12 + slice, st;
    select_part(PART_BRK);
    bp = &proj.pat[seq.ppat[TRK_BRK]].brk;
    bp->steps = 0; memset(bp->slice, 0, sizeof bp->slice);
    set_rec(1);
    set_play(1);
    tick(120);
    midi3(0x90 | ch, note, 100); tick(5); midi3(0x80 | ch, note, 0); tick(20);
    for (st = 0; st < 16 && !((bp->steps >> st) & 1u); st++) ;
    CHECK(__builtin_popcount(bp->steps & 0xFFFFu) == 1 && st < 16 && bp->slice[st] == slice + 1,
          "BREAK: own-channel note writes a step with its slice (steps %04X slice %d)", (unsigned)bp->steps,
          st < 16 ? bp->slice[st] : -1);
    bp->steps = 0; memset(bp->slice, 0, sizeof bp->slice);
    midi3(0x90 | other, note, 100); tick(5); midi3(0x80 | other, note, 0); tick(20);
    CHECK(bp->steps == 0, "BREAK: other-channel note writes nothing");
    set_rec(0);
    tick(5);
    midi3(0x90 | ch, note, 100); tick(5); midi3(0x80 | ch, note, 0); tick(20);
    CHECK(bp->steps == 0, "BREAK: REC off: nothing written");
    set_play(0);
    printf("  BREAK external MIDI record: own channel writes, other channel and REC off do not: checked\n");
}

static int bass_notes(int b)
{
    int n = 0;
    const bpart_t *bp = &proj.pat[seq.ppat[PART_303A + b]].bass[b];
    for (int s = 0; s < NSTEPS; s++) n += (bp->step[s].flags & BS_GATE_MASK) != 0;
    return n;
}
static void test_all_parts(const char *bank)
{
    select_part(PART_303A);
    set_view(V_HOME); build_pages(); tick(50);
    clear_drum(0); clear_drum(1);
    memset(&proj.pat[seq.ppat[PART_BRK]].brk, 0, sizeof proj.pat[0].brk);
    for (int b = 0; b < 2; b++) memset(proj.pat[seq.ppat[PART_303A + b]].bass[b].step, 0, sizeof proj.pat[0].bass[0].step);
    int b0 = bass_notes(0), b1 = bass_notes(1);
    set_rec(1); set_play(1);
    CHECK(ui.rec && seq.playing && ui.view == V_HOME, "%s all parts: REC on, playing, HOME view", bank);
    int c909 = seq.ch[TRK_DRUM], c808 = seq.ch[TRK_DRUM + 1], cbrk = seq.ch[TRK_BRK];
    int ca = seq.ch[TRK_BASS0], cb = seq.ch[TRK_BASS0 + 1];
    for (int i = 0; i < 4; i++) {
        midi3(0x90 | c909, GM_DRUM[0][2], 90); midi3(0x90 | c808, GM_DRUM[1][2], 90);
        midi3(0x90 | cbrk, 14, 100); midi3(0x90 | ca, 48 + i, 90); midi3(0x90 | cb, 55 + i, 90);
        tick(60);
        midi3(0x80 | c909, GM_DRUM[0][2], 0); midi3(0x80 | c808, GM_DRUM[1][2], 0);
        midi3(0x80 | cbrk, 14, 0); midi3(0x80 | ca, 48 + i, 0); midi3(0x80 | cb, 55 + i, 0);
        tick(200);
    }
    CHECK(drum_hits(0, 2) > 0, "%s all parts: 909 records while 303A is selected on HOME", bank);
    CHECK(drum_hits(1, 2) > 0, "%s all parts: 808 records at the same time", bank);
    CHECK(proj.pat[seq.ppat[PART_BRK]].brk.steps != 0, "%s all parts: BREAK records at the same time", bank);
    CHECK(bass_notes(0) > b0, "%s all parts: 303A records its channel off SEQ", bank);
    CHECK(bass_notes(1) > b1, "%s all parts: 303B records at the same time", bank);
    set_play(0); set_rec(0);
    printf("  %s: every part records its own channel at once (909, 808, BREAK, A, B): checked\n", bank);
}

/* QA #734 (a8d9019f): an onset queued in rec_q keeps the REC context it came in.
 * Audio runs alone here (no UI frame), so the onset waits in the queue while
 * the context changes; the next UI frame must not write it. */
static void audio_only(void)
{
    static int32_t b[512];
    engine_render(b, 256);
}
static void test_stale_onsets(void)
{
    int k = 0, v = 2, note = GM_DRUM[k][v], ch = seq.ch[TRK_DRUM + k], old, other;
    /* 1. REC off at the event, REC on before the drain */
    select_part(k); clear_drum(k);
    set_play(1); tick(100);
    midi3(0x90 | ch, note, 90); audio_only(); midi3(0x80 | ch, note, 0);
    set_rec(1); tick(20);
    CHECK(drum_hits(k, v) == 0, "stale: onset before REC on is not written (steps %d)", drum_hits(k, v));
    /* control: the same onset with REC on is written */
    midi3(0x90 | ch, note, 90); tick(5); midi3(0x80 | ch, note, 0); tick(20);
    CHECK(drum_hits(k, v) == 1, "stale control: REC on onset writes one step (steps %d)", drum_hits(k, v));
    /* 2. pattern change before the drain */
    old = seq.ppat[k]; other = (old + 1) % NPAT;
    memset(proj.pat[other].drum[k].hit, 0, sizeof proj.pat[0].drum[0].hit);
    midi3(0x90 | ch, note, 90); audio_only(); midi3(0x80 | ch, note, 0);
    seq.ppat[k] = (uint8_t)other;
    tick(20);
    CHECK(drum_hits(k, v) == 0, "stale: onset of P%d not written to P%d (steps %d)", old + 1, other + 1, drum_hits(k, v));
    seq.ppat[k] = (uint8_t)old; tick(20);
    /* 3. STOP then START before the drain */
    clear_drum(k);
    midi3(0x90 | ch, note, 90); audio_only(); midi3(0x80 | ch, note, 0);
    seq.req_stop = 1; audio_only(); seq.req_start = 1; audio_only();
    tick(20);
    CHECK(seq.playing, "stale: playing again after STOP and START");
    CHECK(drum_hits(k, v) == 0, "stale: onset of the old run not written in the new run (steps %d)", drum_hits(k, v));
    set_rec(0); set_play(0);
    printf("  external MIDI REC queue: REC on, pattern change and STOP/START drop stale onsets: checked\n");
}

int main(void)
{
    boot();
    tick(300);
    seq.bpm = 120.0f;
    test_drum(0, "909");
    test_drum(1, "808");
    test_break();
    test_stale_onsets();
    for (int bank = 0; bank < 2; bank++) {   /* Chicago A + B, then Chicago A + Miami B */
        CHECK(!engine_bass_select_preset(0, 0, 0, &proj.sound), "Chicago on A");
        CHECK(!engine_bass_select_preset(1, bank, 0, &proj.sound), "bank on B");
        tick(100);
        test_all_parts(bank ? "Chicago+Miami" : "Chicago+Chicago");
    }
    printf("ext_midi_rec: %s\n", fails ? "FAIL" : "ok");
    return fails != 0;
}
