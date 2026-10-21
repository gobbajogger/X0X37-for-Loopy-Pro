/* SPDX-License-Identifier: GPL-3.0-only */
/* Production UI, motion and DSP state: pattern ownership, lock deletion,
 * Clear Part probability and reverse conditional passes. */
#define main simulator_main
#include X0X_HOST_SOURCE
#undef main
#include X0X_ENGINE_SOURCE

static unsigned checks, failures;
#define CHECK(c) do { checks++; if (!(c)) { failures++; fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static void render_frames(unsigned n)
{
    int32_t out[512];
    while (n) {
        unsigned k = n > 256 ? 256 : n;
        engine_render(out, k);
        n -= k;
    }
}

static void test_pattern_switch(void)
{
    int a, b, s;
    boot();
    proj.pat[0].drum[0].len = proj.pat[1].drum[0].len = 2;
    proj.pat[0].bass[0].len = proj.pat[1].bass[0].len = 2;
    a = motion_alloc(proj.arr.lane, 0, PART_303A, T_303, 0, BASS303_CUTOFF);
    b = motion_alloc(proj.arr.lane, 1, PART_303A, T_303, 0, BASS303_CUTOFF);
    CHECK(a >= 0 && b >= 0);
    for (s = 0; s < NSTEPS; s++) {
        proj.arr.lane[a].val[s] = 20;
        proj.arr.lane[b].val[s] = 100;
    }
    seq_cue(&seq, 1);
    seq_start(&seq);
    render_frames(1);
    CHECK(b303[0].pot[BASS303_CUTOFF] == 100);
    seq_cue_part(&seq, TRK_BASS0, 0);
    render_frames(22052);
    CHECK(seq.ppat[TRK_BASS0] == 0);
    CHECK(b303[0].pot[BASS303_CUTOFF] == 20 && engine_motion_value(a) == 20);
    render_frames(FS);
    CHECK(b303[0].pot[BASS303_CUTOFF] == 20);
}

static void test_clear_part_probability(void)
{
    int k;
    boot();
    ui.part = PART_909;
    set_view(V_PART);
    k = motion_alloc(proj.arr.lane, 0, PART_909, T_PROB, PART_909 << 4, 0);
    CHECK(k >= 0);
    proj.arr.lane[k].val[0] = 5;
    seq_start(&seq);
    render_frames(1);
    CHECK(prob[PART_909][0] == 5);
    run_action(ACT_CLEAR_PART, PART_909);
    seq_stop(&seq);
    render_frames(1);
    drum_step(PART_909, 0, 1);
    drum_step(PART_909, 0, 0);
    seq_start(&seq);
    render_frames(1);
    CHECK(prob[PART_909][0] == 100 && motion_count(proj.arr.lane) == 0);
}

static void held_303(void)
{
    ui.part = PART_303A;
    set_view(V_PART);
    ui.held_step = 0;
    ui.held_step_part = PART_303A;
    ui.held_step_pat = 0;
}

static void test_remove_last_lock(void)
{
    int k, s;
    boot();
    held_303();
    k = motion_alloc(proj.arr.lane, 0, PART_303A, T_303, 0, BASS303_CUTOFF);
    CHECK(k >= 0);
    proj.arr.lane[k].val[0] = 20;
    seq_start(&seq);
    render_frames(1);
    CHECK(b303[0].pot[BASS303_CUTOFF] == 20);
    ui.btn = 1u << B_SAVE;
    lock_turn(PR(R_ENG, T_303, 0, BASS303_CUTOFF), 1, EN_K1);
    ui.btn = 0;
    render_frames(1);                                  /* restore at the next block, before the next step */
    CHECK(b303[0].pot[BASS303_CUTOFF] == proj.sound.v[T_303][0][BASS303_CUTOFF]);
    render_frames(6000);
    seq_stop(&seq);
    render_frames(1);
    CHECK(b303[0].pot[BASS303_CUTOFF] == proj.sound.v[T_303][0][BASS303_CUTOFF]);
    CHECK(motion_count(proj.arr.lane) == 0);

    boot();
    held_303();
    k = motion_alloc(proj.arr.lane, 0, PART_303A, T_PROB, PART_303A << 4, 0);
    CHECK(k >= 0);
    proj.arr.lane[k].val[0] = 5;
    seq_start(&seq);
    render_frames(1);
    CHECK(prob[PART_303A][0] == 5);
    for (s = 0; s < 19; s++)
        prob_turn(1);                                 /* SELECT turns the final lock up to 100% */
    CHECK(motion_count(proj.arr.lane) == 0);
    render_frames(1);
    CHECK(prob[PART_303A][0] == 100);
}

static void test_reverse_condition(void)
{
    int k, s;
    boot();
    seq.bpm = 120.0f;
    proj.pat[0].bass[0].len = 4;
    proj.pat[0].bass[0].dir = DIR_REV;
    k = motion_alloc(proj.arr.lane, 0, PART_303A, T_PROB, PART_303A << 4, 0);
    CHECK(k >= 0);
    for (s = 0; s < 4; s++) {
        proj.pat[0].bass[0].step[s] = (bstep_t){36, G_NOTE, 100};
        proj.arr.lane[k].val[s] = PROB_COND;             /* 1:2, every note of alternate complete passes */
    }
    seq_start(&seq);
    render_frames(1);
    for (s = 0; s < 12; s++) {
        CHECK(seq.t[TRK_BASS0].pos == 3 - s % 4);
        CHECK(seq.t[TRK_BASS0].bars == s / 4);
        CHECK(b303[0].gate == !(s / 4 % 2));
        render_frames((seq.t[TRK_BASS0].to_next + 255u) / 256u);
    }
}

int main(void)
{
    test_pattern_switch();
    test_clear_part_probability();
    test_remove_last_lock();
    test_reverse_condition();
    printf("motion UI/engine: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
