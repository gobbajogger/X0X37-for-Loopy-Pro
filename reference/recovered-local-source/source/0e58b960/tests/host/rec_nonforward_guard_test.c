/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual MIDI recorder admission in repeated-cell directions.
 * Refusal preserves pattern bytes, normal audition and transport.
 * Exact PINGPONG/RANDOM capture remains an explicit separate design obligation. */
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
    /* REC only admits new notes from visible SEQ after the SEQ/EDIT split. */
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
static void test_nonforward_guard(void) {
 bpart_t before;
 for(int dir=DIR_PINGPONG;dir<=DIR_RANDOM;dir++) {
  capture_reset(4,0);proj.pat[0].bass[0].dir=(uint8_t)dir;
  seq_stop(&seq);capture_frames(1);seq_start(&seq);capture_frames(1);capture_clock=0;
  before=proj.pat[0].bass[0];
  capture_midi(441,0,48,90);
  CHECK(engine_midi_held()&1,"unsupported recorder still auditions accepted live press");
  CHECK(!seq.rec_bass[0].active,"unsupported repeated-cell mode admits no capture");
  CHECK(seq.playing,"record refusal does not stop normal transport");
  CHECK(!memcmp(&before,&proj.pat[0].bass[0],sizeof before),"record admission refusal writes no pattern byte");
  CHECK(!strcmp(ui.msg,"REC NEEDS FWD/REV"),"unsupported mode states its actual limitation");
  capture_midi(28000,0,48,0);
  CHECK(!memcmp(&before,&proj.pat[0].bass[0],sizeof before),"release after refusal cannot partially mutate pattern");
 }
}
int main(void) {
 load_force=0;boot();run_ms(300);test_nonforward_guard();
 printf("rec_nonforward_guard: %u checks %u failures\n",checks,fails);return fails!=0;
}
