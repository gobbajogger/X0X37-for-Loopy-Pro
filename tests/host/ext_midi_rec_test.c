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

/* QA #734: an onset queued in rec_q keeps the REC context it came in.
 * Audio runs alone here (no UI frame), so the onset waits in the queue while
 * the context changes; the next UI frame must not write it. */
static void audio_only(void)
{
    static int32_t b[512];
    engine_render(b, 256);
}
/* Keep audio and UI scheduling separate at each recorder boundary. */
static int queued_part_hits(int part)
{
    return part < NKIT ? drum_hits(part, 2) :
        __builtin_popcount(proj.pat[seq.ppat[PART_BRK]].brk.steps & 0xffffu);
}
static void queued_part_clear(int part)
{
    if (part < NKIT) clear_drum(part);
    else memset(&proj.pat[seq.ppat[PART_BRK]].brk, 0, sizeof proj.pat[0].brk);
}
static void queue_part_voice(int part, int voice)
{
    int ch = seq.ch[part], note = part < NKIT ? GM_DRUM[part][voice] : 12 + voice;
    midi3(0x90 | ch, note, 90); audio_only();
    midi3(0x80 | ch, note, 0); audio_only();
}
static void queue_part_onset(int part) { queue_part_voice(part, 2); }
static int queued_voice_hits(int part, int voice)
{
    if (part < NKIT) return drum_hits(part, voice);
    const brkpart_t *bp = &proj.pat[seq.ppat[part]].brk;
    int n = 0;
    for (int st = 0; st < 16; st++)
        n += ((bp->steps >> st) & 1u) && bp->slice[st] == voice + 1;
    return n;
}
/* Exercise the real audio-owned cue/bar-boundary path without waiting a bar. */
static void cue_part_now(int part, int pattern)
{
    seq_cue_part(&seq, part, pattern);
    seq.t[TRK_DRUM].pos = (uint8_t)(seq_part_len(&seq, TRK_DRUM) - 1);
    seq.t[TRK_DRUM].started = 1;
    seq.t[TRK_DRUM].to_next = 0;
    audio_only();
    CHECK(seq.ppat[part] == pattern, "part %d: queued pattern takes effect", part);
}
static void queued_part_control(int part, const char *name)
{
    queue_part_onset(part); tick(20);
    CHECK(queued_part_hits(part) == 1, "%s: current-context onset writes one step (got %d)", name, queued_part_hits(part));
    queued_part_clear(part);
}
static void test_part_boundaries(int part, const char *name)
{
    int old, other;
    select_part(part); queued_part_clear(part);
    set_play(1); tick(100);
    queue_part_onset(part);
    set_rec(1); tick(20);
    CHECK(queued_part_hits(part) == 0, "%s: REC-off onset does not carry into REC-on", name);
    queued_part_control(part, name);

    queue_part_onset(part);
    set_rec(0); set_rec(1); tick(20);
    CHECK(queued_part_hits(part) == 0, "%s: disarming REC discards the pending onset", name);
    queued_part_control(part, name);

    set_play(0); queue_part_onset(part); set_play(1);
    CHECK(queued_part_hits(part) == 0, "%s: stopped onset does not carry into START", name);
    queued_part_control(part, name);

    old = seq.ppat[part]; other = (old + 1) % NPAT;
    cue_part_now(part, other); queued_part_clear(part);
    cue_part_now(part, old);
    queue_part_onset(part);
    cue_part_now(part, other); tick(20);
    CHECK(queued_part_hits(part) == 0, "%s: old-pattern onset does not write to new pattern", name);
    queued_part_control(part, name);
    cue_part_now(part, old); tick(20);
    CHECK(queued_part_hits(part) == 0, "%s: old-pattern onset did not write to old pattern either", name);


    queue_part_onset(part);
    cue_part_now(part, other); cue_part_now(part, old); tick(20);
    CHECK(queued_part_hits(part) == 0, "%s: pattern away/back before UI drain discards the old onset", name);
    queued_part_clear(part);
    queued_part_control(part, name);

    queue_part_onset(part);
    seq.req_stop = 1; audio_only(); tick(20);
    CHECK(!seq.playing && queued_part_hits(part) == 0, "%s: STOP discards pending onset", name);
    seq.req_cont = 1; audio_only(); tick(20);
    CHECK(seq.playing && queued_part_hits(part) == 0, "%s: stopped onset stays discarded after CONTINUE", name);
    queued_part_control(part, name);

    queue_part_onset(part);
    seq.req_stop = 1; audio_only(); seq.req_cont = 1; audio_only(); tick(20);
    CHECK(seq.playing && queued_part_hits(part) == 0, "%s: STOP/CONTINUE before UI drain discards old-run onset", name);
    queued_part_control(part, name);

    queue_part_onset(part);
    seq.req_stop = 1; audio_only(); seq.req_start = 1; audio_only(); tick(20);
    CHECK(seq.playing && queued_part_hits(part) == 0, "%s: STOP/START before UI drain discards old-run onset", name);
    queued_part_control(part, name);

    queue_part_onset(part);
    seq.req_start = 1; audio_only(); tick(20);
    CHECK(seq.playing && queued_part_hits(part) == 0, "%s: START while playing discards old-run onset", name);
    queued_part_control(part, name);
    set_rec(0); set_play(0);
    printf("  %s REC boundaries: disarm, stopped input, pattern, STOP, START and CONTINUE: checked\n", name);
}

/* A boundary must discard the old onset and retain the first new onset even
 * when both are processed before the next 16-ms UI drain. */
static void test_fresh_boundary_onsets(int part, const char *name)
{
    int old, other;
    select_part(part); queued_part_clear(part);
    set_rec(1); set_play(1); tick(100);
    for (int cont = 0; cont < 2; cont++) {
        queue_part_voice(part, 2);
        seq.req_stop = 1; audio_only();
        if (cont) seq.req_cont = 1; else seq.req_start = 1;
        audio_only(); queue_part_voice(part, 3); tick(20);
        CHECK(queued_voice_hits(part, 2) == 0, "%s: %s discards old-run onset", name, cont ? "CONTINUE" : "START");
        CHECK(queued_voice_hits(part, 3) == 1, "%s: first %s onset survives before UI drain (got %d)", name,
              cont ? "CONTINUE" : "START", queued_voice_hits(part, 3));
        queued_part_clear(part);
    }
    old = seq.ppat[part]; other = (old + 1) % NPAT;
    cue_part_now(part, other); queued_part_clear(part);
    cue_part_now(part, old); tick(20);
    queue_part_voice(part, 2);
    cue_part_now(part, other); queue_part_voice(part, 3); tick(20);
    CHECK(queued_voice_hits(part, 2) == 0, "%s: pattern boundary discards old onset", name);
    CHECK(queued_voice_hits(part, 3) == 1, "%s: first new-pattern onset survives before UI drain (got %d)", name, queued_voice_hits(part, 3));
    queued_part_clear(part);
    cue_part_now(part, old); tick(20);
    CHECK(queued_part_hits(part) == 0, "%s: old-pattern onset was discarded", name);

    queue_part_voice(part, 2);
    set_rec(0); set_rec(1); queue_part_voice(part, 3); tick(20);
    CHECK(queued_voice_hits(part, 2) == 0, "%s: REC off/on discards old onset", name);
    CHECK(queued_voice_hits(part, 3) == 1, "%s: first rearmed REC onset survives before UI drain (got %d)", name, queued_voice_hits(part, 3));
    queued_part_clear(part);
    set_rec(0); set_play(0);
    printf("  %s first new REC/START/CONTINUE/pattern onsets before UI drain: checked\n", name);
}


static void test_transport_generation_alias(void)
{
    select_part(PART_909); clear_drum(0);
    set_rec(1); set_play(1); tick(100);
    for (int count = 128; count <= 256; count *= 2) {
        queue_part_voice(PART_909, 2);
        for (int i = 0; i < count; i++) { seq.req_start = 1; audio_only(); }
        queue_part_voice(PART_909, 3); tick(20);
        CHECK(drum_hits(0, 2) == 0, "generation: old onset stays discarded after %d restarts before UI drain", count);
        CHECK(drum_hits(0, 3) == 1, "generation: fresh onset survives %d restarts before UI drain", count);
        clear_drum(0);
    }
    set_rec(0); set_play(0);
}


/* Reinitializing the engine resets the generation. Its old queue must reset
 * too, even when a queued event has the same generation as the new engine. */
static void test_transport_packet_then_onset(void)
{
    for (int part = 0; part < NPARTS; part++) for (int cont = 0; cont < 2; cont++) {
        if (part >= NKIT && part != PART_BRK) continue;
        select_part(part); queued_part_clear(part); set_rec(1);
        int note = part < NKIT ? GM_DRUM[part][2] : 14;
        seq.clk_src = SEQ_CLK_AUTO;
        midi3(cont ? 0xFB : 0xFA, 0, 0);
        midi3(0x90 | seq.ch[part], note, 90);
        audio_only();
        midi3(0x80 | seq.ch[part], note, 0); audio_only(); tick(20);
        CHECK(queued_part_hits(part) == 1, "MIDI packet order: part %d first onset after %s in same input batch survives (got %d)",
              part, cont ? "CONTINUE" : "START", queued_part_hits(part));
        set_rec(0); set_play(0);
    }
}

static void test_queue_reset(void)
{
    uint32_t ev;
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    seq.playing = 1;
    seq.rec_ext_mask = (uint8_t)((1u << NBASS) - 1u);
    queue_part_onset(PART_909);
    CHECK(engine_rec_ev_get(&ev), "reset: an onset can be queued before engine reinitialization");
    queue_part_onset(PART_909);
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    CHECK(!engine_rec_ev_get(&ev), "reset: no queued onset survives engine reinitialization");
}

int main(void)
{
    boot();
    tick(300);
    seq.bpm = 120.0f;
    test_drum(0, "909");
    test_drum(1, "808");
    test_break();
    for (int part = 0; part < NPARTS; part++) {
        if (part >= NKIT && part != PART_BRK) continue;
        const char *name = part == PART_909 ? "909" : part == PART_808 ? "808" : "BREAK";
        test_part_boundaries(part, name);
        test_fresh_boundary_onsets(part, name);
    }
    test_transport_generation_alias();
    for (int bank = 0; bank < 2; bank++) {   /* Chicago A + B, then Chicago A + Miami B */
        CHECK(!engine_bass_select_preset(0, 0, 0, &proj.sound), "Chicago on A");
        CHECK(!engine_bass_select_preset(1, bank, 0, &proj.sound), "bank on B");
        tick(100);
        test_all_parts(bank ? "Chicago+Miami" : "Chicago+Chicago");
    }
    test_transport_packet_then_onset();
    test_queue_reset();
    printf("ext_midi_rec: %s\n", fails ? "FAIL" : "ok");
    return fails != 0;
}
