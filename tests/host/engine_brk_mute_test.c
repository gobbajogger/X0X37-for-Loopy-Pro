/* SPDX-License-Identifier: GPL-3.0-only */
/* Production engine control functions are extracted verbatim by the Python
 * runner. The real breaks.c supplies decoding, held keys, generator and gate.
 * Other sound owners, master/FX and sequencer timing are inert test boundaries;
 * step delivery is explicit so a slow BREAK cannot conceal a late mute. */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"
#define X0X_IMA_TABLES_DEFINE
#include "../../firmware/src/dsp/breaks.c"

static seq_t seq;
static breaks_t brk;
static uint8_t brk_applied[BRK_NSET];
static uint32_t brk_local;
static int fail_count, click_failed, probability_skip;
static unsigned split_frames = 256, advance_calls, elapsed_frames;
static unsigned scheduled_mute_at, scheduled_unmute_at;
static float captured[256];
static unsigned captured_count, param_count;
static uint32_t seen_param[4096];

/* Unrelated owners only. CQ, BREAK local keys, step and render control below
 * are production bodies, not reimplementations. */
static uint8_t bass_held[NBASS], bass_held_count[NBASS], bass_held_src[NBASS];
static uint8_t original_key_pitch[NBASS][38], bass_attacked[NBASS];
static uint16_t src_held[3];
static int original_bass[NBASS], brk_bend_owner, project_midi_drop;
static int16_t brk_wheel, brk_wheel_seen;
static uint8_t project_hold, tape_signal_seen, usb_full = 1;
static uint8_t mot_was_on, mot_on, mot_release_req, mot_playing;
static unsigned eng_prof_frames, eng_scope_w;
static int16_t eng_scope[512];
static uint16_t eng_peak[5];
static float part_peak[5], out_lim = 1.0f, out_scale = 4194304.0f;
static int mot, mst, fx;
#define SCOPE_N 512u
#define ORIGINAL_INPUT_KEYS 1
#define PROF_T(name) unsigned name = 0
#define PROF_ADD(stage, name) ((void)(name))

static int prob_skip(int part, int voice) { return probability_skip; }
static void original_midi_context(void) {}
static void brk_channel_reconcile(void) {}
static void original_prepare_all(void) {}
static void guard_apply(void) {}
static void live_key_drain(void) {}
static void brk_midi_drain(void) {}
static void motion_release(int *owner) {}
static void motion_tick(int *owner, uint32_t n) {}
static void fxbus_set_bpm(int *owner, float bpm) {}
static void master_process_st(int *owner, float *l, float *r, int n, float volume) {}
uint32_t plat_master(void) { return 4096; }
int plat_midi_in(uint32_t *packet) { return 0; }
static void midi_in(uint32_t packet) {}
static void apply_param(int type, int a, int b, int value)
{
    if (param_count < sizeof seen_param / sizeof seen_param[0])
        seen_param[param_count++] = (uint32_t)value;
}
static void mod_apply(int a, int v) {}
static void apply_base(int a, int b, int v) {}
static int mot_base(void *x, int a, int b, int v) { return 0; }
static void mot_apply(void *x, int a, int b, int v, int base) {}
static void s_drum(void *x, int a, int b, float velocity) {}
static int live_rec_legato(int a, int source) { return 0; }
static void live_rec_accept(int a, int source, int note) {}
static int original_runtime_key(int *owner, int source, unsigned note, unsigned velocity) { return 0; }
static int original_runtime_release_source(int *owner, int source) { return 0; }
void seq_rec_bass(seq_t *s, int b, int note, int velocity, int down, int source, int flags) {}
float seq_tempo(const seq_t *s) { return 20.0f; }
int seq_clock_waiting(const seq_t *s) { return 0; }
/* No external notes in this harness; local key ownership is production code. */
static int brk_any_slice(int slice) { return (brk_local & (1u << slice)) != 0; }
static void s_brk(void *, int, int, float, int);
static const seq_sink_t SINK = {.brk = s_brk};

uint32_t seq_until_event(seq_t *s, uint32_t n)
{
    unsigned k = n < split_frames ? n : split_frames;
    if (scheduled_mute_at > elapsed_frames && scheduled_mute_at - elapsed_frames < k)
        k = scheduled_mute_at - elapsed_frames;
    if (scheduled_unmute_at > elapsed_frames && scheduled_unmute_at - elapsed_frames < k)
        k = scheduled_unmute_at - elapsed_frames;
    return k;
}

void seq_advance(seq_t *s, uint32_t n, const seq_sink_t *sink)
{
    elapsed_frames += n;
    ++advance_calls;
    if (scheduled_mute_at && elapsed_frames == scheduled_mute_at)
        s->mute |= 1u << MUTE_BRK;
    if (scheduled_unmute_at && elapsed_frames == scheduled_unmute_at)
        s->mute &= ~(1u << MUTE_BRK);
}

static void render_sub(float *l, float *r, uint32_t n, uint32_t offset)
{
    /* Real DSP output captured before unrelated effects/master/mixer. */
    breaks_render(&brk, l, (int)n);
    assert(captured_count + n <= 256u);
    for (unsigned i = 0; i < n; ++i) {
        r[i] = l[i];
        captured[captured_count++] = l[i];
    }
}

#include "production_engine_brk_control.inc"

#define CHECK(condition, message) do { \
    if (!(condition)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, message); ++fail_count; } \
} while (0)

/* Standard IMA encoder, test data only: a positive plateau makes an abrupt cut
 * a measurable discontinuity without relying on a fortunate sine phase. */
#define LOOP_N 88200u
static uint8_t loop_data[LOOP_N / 2u];
static const brk_loop_t loop = {loop_data, LOOP_N, 22050, 2, 8};
static pattern_t pattern;

static void make_loop(void)
{
    int pred = 0, index = 0;
    for (unsigned i = 0; i < LOOP_N; ++i) {
        int step = IMA_STEP_T[index], diff = 20000 - pred, code = 0, delta = step >> 3;
        if (diff < 0) { code = 8; diff = -diff; }
        if (diff >= step) { code |= 4; diff -= step; delta += step; }
        if (diff >= (step >> 1)) { code |= 2; diff -= step >> 1; delta += step >> 1; }
        if (diff >= (step >> 2)) { code |= 1; delta += step >> 2; }
        pred += code & 8 ? -delta : delta;
        if (pred > 32767) pred = 32767;
        if (pred < -32768) pred = -32768;
        index += IMA_IDX_T[code & 7];
        if (index < 0) index = 0;
        if (index > 88) index = 88;
        loop_data[i >> 1] |= (uint8_t)(code << ((i & 1u) * 4u));
    }
}

static void block(void)
{
    int32_t out[512];
    captured_count = 0;
    engine_render(out, 256);
    CHECK(captured_count == 256, "real engine_render rendered exactly one block");
}

static float peak(unsigned start, unsigned end)
{
    float value = 0.0f;
    for (unsigned i = start; i < end; ++i)
        if (fabsf(captured[i]) > value) value = fabsf(captured[i]);
    return value;
}

static void fixture(int running)
{
    memset(&seq, 0, sizeof seq);
    memset(&pattern, 0, sizeof pattern);
    memset(bass_held, 0, sizeof bass_held);
    memset(bass_held_count, 0, sizeof bass_held_count);
    memset(bass_held_src, 0, sizeof bass_held_src);
    memset(brk_applied, 255, sizeof brk_applied);
    cq_w = cq_r = brk_local = param_count = 0;
    elapsed_frames = advance_calls = scheduled_mute_at = scheduled_unmute_at = 0;
    split_frames = 256;
    probability_skip = 0;
    seq.pat = &pattern;
    seq.playing = (uint8_t)running;
    pattern.brk.steps = 0xFFFFu;
    breaks_init(&brk);
    breaks_set_loop(&brk, 0, &loop);
    breaks_set(&brk, BRK_LEVEL, 100);
    breaks_set(&brk, BRK_PITCH, 12);
    for (int i = 0; i < BRK_NSET; ++i) pattern.brk.set[i] = (uint8_t)breaks_get(&brk, i);
    pattern.brk.set[BRK_COMPLEX] = 0;
    pattern.brk.set[BRK_ALEN] = 3;
    if (running) s_brk(0, 0, 0, 33075.0f, 1); /* 20 BPM: no next step for >129 blocks */
    for (int i = 0; i < 4; ++i) block();
    elapsed_frames = advance_calls = 0;
}

static void test_quiet_alone(void)
{
    fixture(1);
    breaks_set_quiet(&brk, 1);
    block();
    CHECK(peak(132, 256) > 0.1f, "characterization: quiet alone does not mute an open gate");
}

static void test_deadline_and_click(void)
{
    fixture(1);
    const float previous = captured[255];
    CHECK(previous > 0.1f, "pre-mute deterministic plateau is audibly nonzero");
    const uint32_t triggers = brk.st_trigs;
    seq.mute |= 1u << MUTE_BRK;
    block();
    CHECK(brk.st_trigs == triggers, "deadline does not wait for or manufacture a BREAK step");
    CHECK(brk.gate == 0.0f && peak(132, 256) == 0.0f, "mute is quiet within the next 256 frames");
    CHECK(peak(0, 132) > 0.0f || BRK_FADE == 1, "normal mute fades rather than cutting immediately");
    float delta = fabsf(captured[0] - previous);
    for (unsigned i = 1; i < 256; ++i)
        if (fabsf(captured[i] - captured[i - 1]) > delta) delta = fabsf(captured[i] - captured[i - 1]);
    const float bound = previous * 0.025f + 0.0001f;
    click_failed = delta > bound;
    printf("mute peak-step %.7f, bound %.7f; prior %.7f; late peak %.7f\n",
           (double)delta, (double)bound, (double)previous, (double)peak(132, 256));
#ifndef ENGINE_BRK_ABRUPT_CONTROL
    CHECK(!click_failed, "declick peak-step bound, including previous block's final sample");
#endif
}

static void test_unmute_and_splits(void)
{
    fixture(1);
    split_frames = 17;
    seq.mute |= 1u << MUTE_BRK;
    block();
    CHECK(peak(132, 256) == 0.0f, "declick completes across uneven engine render stretches");
    seq.mute &= ~(1u << MUTE_BRK);
    block();
    CHECK(peak(0, 256) == 0.0f, "unmute cannot resurrect a previous enabled step");
    probability_skip = 1;
    s_brk(0, 1, 0, 33075.0f, 1);
    block();
    CHECK(peak(0, 256) == 0.0f, "unmute preserves next-step probability rejection");
    probability_skip = 0;
    s_brk(0, 2, 0, 33075.0f, 1);
    block();
    CHECK(peak(132, 256) > 0.1f, "unmute opens on the next accepted enabled BREAK step");
    seq.mute |= 1u << MUTE_BRK;
    seq.mute &= ~(1u << MUTE_BRK);
    block();
    CHECK(peak(132, 256) > 0.1f, "latest unmuted state wins before a boundary; no stale mute command");
    seq.mute |= 1u << MUTE_BRK;
    block();
    seq.mute &= ~(1u << MUTE_BRK);
    seq.mute |= 1u << MUTE_BRK;
    block();
    CHECK(peak(0, 256) == 0.0f, "latest muted state wins after rapid toggle");
}

static void test_event_mute(void)
{
    fixture(1);
    scheduled_mute_at = 17;
    scheduled_unmute_at = 201;
    block();
    CHECK(peak(0, 17) > 0.1f, "fixture presents mute at an in-buffer sequencer event");
    CHECK(peak(17 + 132, 256) == 0.0f, "song-event mute fades before the next render stretch ends");
    CHECK(!(seq.mute & (1u << MUTE_BRK)), "fixture delivered subsequent unmute in the same buffer");
    CHECK(brk.gate_to == 0.0f, "in-buffer unmute still waits for an enabled step");
}

static void test_queue_backlog(void)
{
    for (int protected = 0; protected < 2; ++protected) {
        fixture(1);
        for (unsigned i = 0; i < CQ; ++i)
            cq_put(protected ? C_BRK : C_PARAM_T0, protected ? 32 : 0, 0, i);
        CHECK(cq_w - cq_r == CQ, "production CQ is saturated before mute");
        seq.mute |= 1u << MUTE_BRK;
        block();
        CHECK(cq_w - cq_r == CQ - 128u, "production CQ drain preserves its bounded 128-command budget");
        CHECK(peak(132, 256) == 0.0f, "mute bypasses full CQ of parameters or protected releases");
        seq.mute &= ~(1u << MUTE_BRK);
        for (int k = 0; k < 8; ++k) block();
        CHECK(cq_w == cq_r && peak(0, 256) == 0.0f, "old CQ backlog cannot replay mute/unmute state");
        s_brk(0, 2, 0, 33075.0f, 1);
        block();
        CHECK(peak(132, 256) > 0.1f, "normal next-step unmute after CQ backlog clears");
    }
    /* Real FIFO marker boundaries establish that the queue fixture actually
     * exposes the delayed delivery that a mute command would have suffered. */
    fixture(1);
    for (unsigned i = 0; i < CQ; ++i) cq_put(C_PARAM_T0, 0, 0, i);
    block();
    CHECK(param_count == 128u && seen_param[127] == 127u, "position 127 drains on boundary one");
    block();
    CHECK(param_count == 256u && seen_param[128] == 128u, "position 128 waits until boundary two");
    for (int i = 0; i < 6; ++i) block();
    CHECK(param_count == 1024u && seen_param[1023] == 1023u, "position 1023 waits until boundary eight");
}

static void test_held_and_stopped(void)
{
    for (int running = 0; running < 2; ++running) {
        fixture(running);
        if (!running) {
            pattern.brk.steps = 0;
            breaks_set_quiet(&brk, 1); /* prior zero-step pattern, stopped audition remains allowed */
        }
        cq_put(C_BRK, 32 + 3, 1, 0);
        block();
        CHECK(brk.perf.count == 1 && brk.current_slice == 3 && peak(132, 256) > 0.1f,
              "production local CQ press auditions the held slice");
        seq.mute |= 1u << MUTE_BRK;
        block();
        CHECK(brk.perf.count == 1 && peak(132, 256) > 0.1f,
              "mute preserves established held-slice override while running or stopped");
        cq_put(C_BRK, 32 + 3, 0, 0);
        block();
        CHECK(brk.perf.count == 0 && peak(132, 256) == 0.0f,
              "held-slice release fades without reopening the muted old step");
        cq_put(C_BRK, 32 + 5, 1, 0);
        block();
        CHECK(brk.current_slice == 5 && peak(132, 256) > 0.1f,
              "a fresh local key can audition while muted after gate settled");
        cq_put(C_BRK, 32 + 5, 0, 0);
        block();
        CHECK(peak(132, 256) == 0.0f, "fresh audition releases quietly");
    }
    fixture(1);
    cq_put(C_BRK, 32 + 2, 1, 0); /* pending press at the same mute boundary */
    seq.mute |= 1u << MUTE_BRK;
    block();
    CHECK(brk.perf.count == 1 && peak(132, 256) > 0.1f,
          "perf_sync still applies a pending held-slice press after mute boundary");
}

int main(void)
{
    make_loop();
#ifdef ENGINE_BRK_ABRUPT_CONTROL
    test_deadline_and_click();
    if (fail_count) return 1;
    if (!click_failed) { fprintf(stderr, "FAIL abrupt-cut positive control did not fire\n"); return 1; }
    puts("abrupt-cut positive control FIRED");
    return 3;
#else
    test_quiet_alone();
    test_deadline_and_click();
    test_unmute_and_splits();
    test_event_mute();
    test_queue_backlog();
    test_held_and_stopped();
    printf("engine-brk-mute: %s\n", fail_count ? "FAIL" : "PASS");
    return fail_count ? 1 : 0;
#endif
}
