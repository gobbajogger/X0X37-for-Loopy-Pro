/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual MIDI/keyboard command -> engine recorder -> production sequencer regression.
 * Adapted timestamped lifecycle proof from 6496003d onto current source.
 * Disconnected SMC is a platform boundary, not a substituted recorder. */
#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1 /* expose the simulator's libproc instrumentation declarations */
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
void smc_pad_diag_boundary(void) {} /* disconnected SMC fixture */
int smc_midi_host(uint32_t pkt) { (void)pkt; return 0; }
/* Presentation lives in smc_mixer.c; this fixture is disconnected SMC / live-rec only. */
void smc_ui_cancel(void) {}
int smc_ui_active(void) { return 0; }
void smc_ui_draw(void) {}
void smc_ui_clip_show(int n, const char *name, const char *value)
{ (void)n; (void)name; (void)value; }
#endif

static unsigned checks, fails;
#define CHECK(c,...) do { checks++; if(!(c)) { fails++; printf("FAIL line %d: ",__LINE__);printf(__VA_ARGS__);puts(""); } } while(0)
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
    /* Recording is armed from SEQ; EDIT sound controls must not write hidden
     * steps while auditioning notes. */
    set_view(V_SEQ);
    for (int p = 0; p < NPAT; p++) pattern_init(&proj.pat[p], 1, 2);
    proj.pat[0].bass[0].len = (uint8_t)len;
    proj.pat[0].swing = (uint8_t)swing;
    seq_init(&seq, proj.pat); seq.bpm = 120; seq.clk_src = SEQ_CLK_INT;
    seq.send_notes = seq.send_clock = 0;
    input(); seq_start(&seq); capture_frames(1);
    capture_clock = 0; capture_energy = 0;
}
int engine_bass_attacked(int part);
static void capture_until_gate(unsigned limit)
{
    while (capture_clock < limit && !(engine_midi_held() & 1))
        capture_frames(64);
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
    engine_bass_record(0, 48, 90, 0);
    capture_frames(2300);
    CHECK(note_ons == prior_ons + 1 && seq.bass_out_valid[0], "physical recording echoes snapped future onset once without internal retrigger");
    engine_bass_off(0); capture_frames(1);
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
    CHECK(project_load() == 0 && !memcmp(bp, &recorded, sizeof recorded), "current stream6 save/load preserves exact capture bytes");


    CHECK(get32(store[OBJ_PAT0]) == PROJ_STREAM, "new save names current endpoint-compatible stream");
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
    engine_bass_record(0, 48, 90, 0); /* admitted edge, not drained yet */
    run_action(ACT_CLEAR_PART, PART_303A);
    input(); capture_frames(1);
    CHECK(!seq.rec_bass[0].active && bstep_gate(&bp->step[0]) == G_REST,
          "bulk edit invalidates pending keyboard recording epoch before re-arm");
    ui.rec = 0;

    /* Context cancel, then the pattern itself reopens the DSP gate. That gate is
     * not the cancelled note: the new press attacks, and a stale off cannot cut it. */
    capture_reset(16, 0);
    bp->step[8].note = 60;
    bp->step[8].flags = G_NOTE;
    bp->step[8].velocity = 100;
    capture_midi(441, 0, 48, 90);
    capture_frames(6000);
    ui.rec = 0; input(); capture_frames(1);
    CHECK(!seq.rec_bass[0].active && !(engine_midi_held() & 1), "context cancel releases the cancelled voice");
    CHECK(!(bp->step[0].flags & BS_SLIDE) && !(bp->step[1].flags & BS_SLIDE),
          "cancel finalizes the held note without a slide");
    capture_until_gate(capture_clock + 50000);
    CHECK(engine_midi_held() & 1, "sequencer reopened a DSP gate after context cancel");
    ui.rec = 1; input();
    capture_midi(capture_clock + 200, 0, 55, 80);
    CHECK(!(bp->step[0].flags & BS_SLIDE) && !(bp->step[1].flags & BS_SLIDE),
          "reopened gate does not store BS_SLIDE from the cancelled note (flags %u %u)",
          bp->step[0].flags, bp->step[1].flags);
    CHECK(!(seq.t[TRK_BASS0].held_note & 0x80u), "reopened gate is not accepted legato ownership");
    CHECK(engine_bass_attacked(0), "new press after cancel attacks");
    CHECK(engine_midi_held() & 1, "new press is gated");
    capture_midi(capture_clock + 200, 0, 48, 0);
    CHECK(engine_midi_held() & 1, "stale cancelled-note off does not cut the new press");
    capture_midi(capture_clock + 200, 0, 55, 0);
    CHECK(!(engine_midi_held() & 1), "new press ends on its own off");

    capture_reset(16, 0);
    bp->step[8].note = 60;
    bp->step[8].flags = G_NOTE;
    bp->step[8].velocity = 100;
    capture_midi(441, 0, 48, 90);
    capture_frames(6000);
    ui.rec = 0; input(); capture_frames(1);
    capture_until_gate(capture_clock + 50000);
    CHECK(engine_midi_held() & 1, "sequencer reopened the gate before a same-pitch press");
    ui.rec = 1; input();
    capture_midi(capture_clock + 200, 0, 48, 80);
    CHECK(!(bp->step[0].flags & BS_SLIDE), "same pitch after a reopened gate does not slide");
    CHECK(engine_bass_attacked(0), "same pitch after cancel still attacks");
    capture_midi(capture_clock + 200, 0, 48, 0); /* the cancelled press's off */
    CHECK(engine_midi_held() & 1, "same-pitch count keeps the new press through the stale off");
    capture_midi(capture_clock + 200, 0, 48, 0);
    CHECK(!(engine_midi_held() & 1), "same-pitch count releases on the new press's own off");
    capture_reset(16, 0);
    engine_bass_record(0, 48, 90, 0); capture_frames(1);
    capture_midi(2000, 0, 55, 90);
    engine_bass_off(0); capture_frames(1); /* older keyboard release, external input owns new note */
    CHECK(engine_midi_held() & 1, "old keyboard off cannot cut the external successor");
    CHECK(seq.rec_bass[0].active && seq.rec_bass[0].src == 0, "external successor retains recording ownership");
    capture_midi(4000, 0, 55, 0);
    CHECK(!(engine_midi_held() & 1), "external successor ends on its own release");

    for (unsigned src = 0; src < 3; src++) {
        capture_reset(16, 0);
        capture_midi(441, src, 48, 90);
        min_q[mi_w++ % MQ] = src << 4; capture_frames(1); /* actual source-disconnect packet */
        CHECK(!seq.rec_bass[0].active && !seq.rec_bass[0].own_ok, "input disconnect finalizes and invalidates captured press");
        CHECK(!(engine_midi_held() & 1), "input disconnect releases its actual voice");
    }
    capture_reset(16, 0);
    capture_midi(441, 3, 48, 90);
    CHECK(!seq.rec_bass[0].active && bstep_gate(&bp->step[0]) == G_REST, "invalid MIDI route cannot alias internal keyboard recorder ownership");

    puts("  timestamped MIDI recording: duration, gate replay, slide, retrigger, route isolation, note zero, wrap, swing, selection, stop and cancelled-gate ownership checked");
}

int main(void) {
 load_force=0;boot();run_ms(300);
 test_timestamped_live_duration();
 printf("rec_endpoint_lifecycle: %u checks %u failures\n",checks,fails);
 return fails!=0;
}
