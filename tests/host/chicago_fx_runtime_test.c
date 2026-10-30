/* SPDX-License-Identifier: GPL-3.0-only */
/* Real command queue, sequencer tempo and engine render with the standard host HAL. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/chicago_fx.h"
#include "../../firmware/src/dsp/bass303.h"
#include "../../firmware/src/dsp/master.h"

extern int chicago_test_idle(int v);
extern int chicago_test_delay(int v);
extern int chicago_test_position(int v);
extern int chicago_test_type(int v);
extern int chicago_test_miami(int v);
extern void chicago_test_save(int v);
extern int chicago_test_unchanged(int v);
extern void chicago_test_impulse(int v);
extern double chicago_test_energy(int v);

static int checks, failures;
static int32_t audio_out[512];
#define CHECK(c) do { checks++; if (!(c)) { failures++; \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); } } while (0)

static double render_frames(unsigned frames)
{
    double e = 0;
    do {
        unsigned n = frames > 256 ? 256 : frames;
        engine_render(audio_out, n);
        for (unsigned i = 0; i < n * 2; ++i) {
            double x = audio_out[i] / 8388608.0;
            e += x * x;
        }
        frames -= n;
    } while (frames);
    return e;
}

static void set_fx(int type, int e1, int e2)
{
    engine_set(T_303, 1, CHI_FX_TYPE, CHI_FX_OFF);
    engine_set(T_303, 1, CHI_FX_TYPE, type);
    engine_set(T_303, 1, CHI_FX_EDIT1, e1);
    engine_set(T_303, 1, CHI_FX_EDIT2, e2);
    render_frames(0); /* Drain the real command queue without advancing effect history. */
}

static void tempo_follows_sequencer(void)
{
    static const int tempo[] = {120, 170, 240, 20, 300};
    static const int delay[] = {2756, 1946, 1378, 2067, 1103};
    seq.clk_src = SEQ_CLK_INT;
    set_fx(CHI_FX_BPM_DLY, 127, 0);
    for (unsigned i = 0; i < sizeof tempo / sizeof tempo[0]; ++i) {
        seq.bpm = (float)tempo[i];
        render_frames(0);
        printf("internal BPM %d: delay %d, expected %d\n",
               tempo[i], chicago_test_delay(1), delay[i]);
        CHECK(chicago_test_delay(1) == delay[i]);
    }
    /* 1225 samples per MIDI clock gives exactly 90 BPM at 44100 Hz. */
    seq.clk_src = SEQ_CLK_EXT;
    render_frames(0);
    for (int i = 0; i < 24 * 10; ++i) {
        min_q[mi_w++ % MQ] = 0xFu | (0xF8u << 8);
        render_frames(1225);
    }
    CHECK(seq.ext);
    CHECK(fabsf(seq_tempo(&seq) - 90.0f) < 0.1f);
    CHECK(abs(chicago_test_delay(1) - 3675) <= 2);
    printf("external BPM %.4f: delay %d\n", seq_tempo(&seq), chicago_test_delay(1));
    seq.clk_src = SEQ_CLK_INT;
    seq.bpm = 120.0f;
    render_frames(0);
}

static void control_edits_preserve_history(void)
{
    for (int type = 0; type < CHI_FX_NTYPES; ++type) {
        if (!chi_fx_is_insert((uint8_t)type)) continue;
        set_fx(type, 100, 70);
        chicago_test_impulse(1);
        chicago_test_save(1);
        engine_set(T_303, 1, CHI_FX_EDIT1, 100);
        render_frames(0);
        CHECK(chicago_test_unchanged(1));
        engine_set(T_303, 1, CHI_FX_EDIT2, 70);
        render_frames(0);
        CHECK(chicago_test_unchanged(1));
        engine_set(T_303, 1, CHI_FX_TYPE, type);
        render_frames(0);
        CHECK(chicago_test_unchanged(1));
    }
    set_fx(CHI_FX_BPM_DLY, 127, 0);
    chicago_test_impulse(1);
    int w = chicago_test_position(1);
    double e = chicago_test_energy(1);
    CHECK(e > 0.0);
    engine_set(T_303, 1, CHI_FX_EDIT1, 126);
    render_frames(0);
    CHECK(chicago_test_position(1) == w);
    CHECK(chicago_test_energy(1) == e);
    engine_set(T_303, 1, CHI_FX_EDIT2, 30);
    render_frames(0);
    CHECK(chicago_test_position(1) == w);
    CHECK(chicago_test_energy(1) == e);
    /* A true algorithm transition must reset the shared union. */
    engine_set(T_303, 1, CHI_FX_TYPE, CHI_FX_SHORT_DLY);
    engine_set(T_303, 1, CHI_FX_TYPE, CHI_FX_BPM_DLY);
    render_frames(0);
    CHECK(chicago_test_energy(1) == 0.0);
    CHECK(chicago_test_position(1) == 0);
#if X0X_DEV_WOBBLE
    /* Chicago's indices overlap Miami controls; those writes must stay on WBL1. */
    CHECK(chicago_test_miami(0));
    chicago_test_save(0);
    engine_set(T_303, 0, CHI_FX_EDIT1, 50);
    render_frames(0);
    CHECK(chicago_test_unchanged(0));
    CHECK(chicago_test_type(0) == CHI_FX_OFF);
#endif
}

static void released_note_tail_continues(void)
{
    set_fx(CHI_FX_BPM_DLY, 127, 0);
    engine_set(T_303, 1, BASS303_AMPMODE, BASS303_AMP_GATE);
    engine_set(T_MIX, PART_303B, MX_REV, 0);
    engine_set(T_MIX, PART_303B, MX_DLY, 0);
    engine_set(T_MST, 0, MST_LIMIT, 0);
    engine_bass_on(1, 48, 0, 0);
    render_frames(50 * 256);
    engine_bass_off(1);
    unsigned blocks = 0;
    do { render_frames(256); blocks++; }
    while (!chicago_test_idle(1) && blocks < 500);
    CHECK(chicago_test_idle(1));
    double before = chicago_test_energy(1);
    CHECK(before > 0.0);
    int w = chicago_test_position(1);
    double audible = render_frames(200 * 256);
    CHECK(audible > 0.0);
    CHECK(chicago_test_position(1) != w);
    CHECK(chicago_test_energy(1) < before);
    printf("released tail: ring %.9g -> %.9g, output energy %.9g\n",
           before, chicago_test_energy(1), audible);
    /* Off retains the idle shortcut, with no dormant ring advancement. */
    engine_set(T_303, 1, CHI_FX_TYPE, CHI_FX_OFF);
    render_frames(0);
    chicago_test_save(1);
    render_frames(256);
    CHECK(chicago_test_unchanged(1));
}

int main(void)
{
    boot();
    seq_stop(&seq);
    render_frames(0);
    tempo_follows_sequencer();
    control_edits_preserve_history();
    released_note_tail_continues();
    printf("Chicago runtime WBL=%d: %d checks, %d failures\n",
           X0X_DEV_WOBBLE, checks, failures);
    return failures != 0;
}
