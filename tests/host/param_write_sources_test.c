/* SPDX-License-Identifier: GPL-3.0-only */
/* B4 attribution: exercise actual producer paths and assert actual state.
 * The wrapper also checks the opt-in host trace for each writer label. */
#define CHICAGO_PRESENTATION_NO_MAIN
#include "chicago_presentation_test.c"

static void wheel_sample(int pitch, int mod)
{
    fm1_in.frames += 400;
    fm1_in.wheel[0] = pitch; fm1_in.wheel[1] = mod;
    fm1_in.wheel_mod_seq++;
    smk_wheels(); audio();
}
int main(int argc, char **argv)
{
    CHECK(argc == 3);
    int voice = atoi(argv[1]), bank = atoi(argv[2]);
    CHECK(voice >= 0 && voice < 2 && bank >= 0 && bank < 2);
    boot(); run_ms(50); engine_guard_enable(0); fixture(voice, bank);
    int parameter = bank ? ORIGINAL_MIAMI_WOBBLE_DEPTH : ORIGINAL_CHICAGO_CUTOFF;
    pref_t r = PR(R_ENG, T_303, voice, parameter);
    for (int i = 0; i < 4; ++i) wheel_sample(550, 615);
    wheel_sample(600, 550);
    CHECK(engine_bass_perf(voice, 0, 1) > 64);
    CHECK(engine_bass_perf(voice, 2, 1) == 57);
    wheel_sample(550, 615);
    CHECK(engine_bass_perf(voice, 0, 1) == 64);

    pref_set(r, 40); audio(); accel_off = 1;
    turn_ref(r, 1, EN_K1); audio();
    CHECK(pref_get(r) == 41 && engine_bass_perf(voice, 2, 0) == 41);
    CHECK(engine_bass_perf(voice, 2, 1) == -1);
    int page = row(R_ENG, parameter), strip = -1;
    CHECK(page >= 0);
    /* SMC knobs follow the SMC part/view context, not the keyboard one. */
    smc_select_part(PART_303A + voice); smc_view(V_PART, page);
    *page_of(V_PART) = (uint8_t)page; smc.knob_bank = 0;
    for (int n = 0; n < 4; ++n) if (smc_same(smc_knob_target(n), r)) strip = n;
    CHECK(strip >= 0);
    smc_knob(strip, 1); audio();
    CHECK(pref_get(r) == 42 && engine_bass_perf(voice, 2, 0) == 42);

    /* Motion owns runtime only: the saved knob remains 42. */
    int lane = motion_alloc(proj.arr.lane, seq.ppat[PART_303A + voice],
                            PART_303A + voice, T_303, voice, parameter);
    CHECK(lane >= 0);
    for (int i = 0; i < NSTEPS; ++i) proj.arr.lane[lane].val[i] = 99;
    engine_motion_reset(); seq.clk_src = SEQ_CLK_AUTO; seq_start(&seq);
    int heard_motion = 0;
    for (int i = 0; i < 100; ++i) { audio(); heard_motion |= engine_bass_perf(voice, 2, 0) == 99; }
    CHECK(heard_motion && pref_get(r) == 42);
    seq_stop(&seq); audio();
    memset(proj.arr.lane, 0, sizeof proj.arr.lane); engine_motion_reset(); audio();
    float restored;
    uint32_t restored_bits;
    CHECK(original_sound_control_value((unsigned)bank, (unsigned)parameter, 42, &restored));
    memcpy(&restored_bits, &restored, sizeof restored_bits);
    CHECK(engine_original_value_bits(voice, (unsigned)parameter) == restored_bits && pref_get(r) == 42);

    int channel = seq.ch[TRK_BASS0 + voice]; CHECK(channel < 16);
    min_q[mi_w++ % MQ] = 0x0Bu | ((uint32_t)(0xB0 | channel) << 8) |
                        (1u << 16) | (87u << 24);
    audio();
    CHECK(engine_bass_perf(voice, 2, 1) == 87 && pref_get(r) == 42);

    /* Use the real normal mixer fader target, not a manufactured synth ref. */
    smc_ui_cancel(); set_view(V_HOME); smc.locked = 0;
    pref_t f = smc_target(0); const x0x_param_t *d = pref_desc(f);
    CHECK(f.kind == R_ENG && d && d->max > 0);
    pref_set(f, 0); audio();
    smc_fader(0, 0); smc_fader(0, 127); audio();
    CHECK(pref_get(f) == d->max);
    printf("PASS %u B4 writer-source checks voice=%d bank=%d\n", checks, voice, bank);
    return 0;
}
