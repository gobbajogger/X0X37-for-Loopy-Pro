/* SPDX-License-Identifier: GPL-3.0-only */
/* B4: real scanner/engine/motion path; fresh process for each voice/bank/REC.
 * Retaining one-count learning without consecutive-sample ownership must fail
 * the repeated-high-outlier case. No generated ADC model replaces the scanner. */
#define CHICAGO_PRESENTATION_NO_MAIN
#include "chicago_presentation_test.c"
#include "smk_wheel_adc.inc"

static void wheel_sample(int pitch, int mod)
{
    fm1_in.frames += 400;
    fm1_in.wheel[0] = pitch;
    fm1_in.wheel[1] = mod;
    fm1_in.wheel_mod_seq++;
    smk_wheels();
    audio();
}
static int mod_lane(int voice, int parameter)
{
    int part = PART_303A + voice;
    return motion_find(proj.arr.lane, seq.ppat[part], part, T_303, voice, parameter);
}
static void isolated_spikes(int voice, int parameter, int expected)
{
    static const int jitter[] = {0, 2, -1, 3, -3, 1, -2, 0};
    for (int i = 0; i < 80; ++i) {
        wheel_sample(550, 645);
        CHECK(engine_bass_perf(voice, 2, 1) == expected);
        /* Foreground loops can poll one published sample repeatedly. */
        for (int poll = 0; poll < 64; ++poll) smk_wheels();
        audio();
        CHECK(engine_bass_perf(voice, 2, 1) == expected);
        for (int j = 0; j < 3; ++j) {
            wheel_sample(550 + jitter[(i + j) & 7], 615 + jitter[(i + j + 3) & 7]);
            CHECK(engine_bass_perf(voice, 2, 1) == expected);
            CHECK(mod_lane(voice, parameter) < 0);
        }
    }
}
int main(int argc, char **argv)
{
    CHECK(argc == 5);
    int voice = atoi(argv[1]), bank = atoi(argv[2]), record = atoi(argv[3]);
    int expand = atoi(argv[4]);
    CHECK(voice >= 0 && voice < 2 && bank >= 0 && bank < 2 && record >= 0 && record < 2);
    boot(); run_ms(50); engine_guard_enable(0); fixture(voice, bank);
    for (int i = 0; i < 8; ++i) wheel_sample(550, 615);
    int parameter = bank ? ORIGINAL_MIAMI_WOBBLE_DEPTH : ORIGINAL_CHICAGO_CUTOFF;
    CHECK(engine_bass_mod_param(voice) == parameter);
    CHECK(engine_bass_perf(voice, 2, 1) == -1);
    if (expand == 1) {
        /* Genuine held endpoints still teach a wider range. After learning
         * 400..645, raw 550 maps by hand to 83*127/233 = 45. */
        for (int i = 0; i < 100; ++i) wheel_sample(550, 645);
        for (int i = 0; i < 100; ++i) wheel_sample(550, 400);
        wheel_sample(550, 550);
        CHECK(engine_bass_perf(voice, 2, 1) == 45);
    } else if (expand == 2) {
        /* The foreground can miss a rest publication between observed highs.
         * Equal values separated by sequence gaps cannot confirm an endpoint. */
        wheel_sample(550, 645);
        for (int i = 0; i < 40; ++i) {
            /* Unobserved rest + high conversions; latest value is still high. */
            fm1_in.wheel_mod_seq += 2u;
            for (int poll = 0; poll < 64; ++poll) smk_wheels();
            audio();
        }
        wheel_sample(550, 615);
        CHECK(engine_bass_perf(voice, 2, 1) == -1);
        CHECK(mod_lane(voice, parameter) < 0);
    } else if (expand == 3) {
        /* UINT16_MAX -> 0 is one fresh conversion, not a missing sample.
         * Fifteen admitted growth steps teach high630. Rest615 then maps to
         * 3*127/133 = 2; losing the wrap step gives 1, suppressed as jitter. */
        fm1_in.wheel_mod_seq = UINT16_MAX - 1u;
        smk_wheels();
        for (int i = 0; i < 16; ++i) wheel_sample(550, 645);
        wheel_sample(550, 615);
        CHECK(engine_bass_perf(voice, 2, 1) == 2);
    } else {
        sound_t before = proj.sound;
        ui.rec = (uint8_t)record; seq.clk_src = SEQ_CLK_AUTO; seq_start(&seq); audio();
        isolated_spikes(voice, parameter, -1);
        /* Real HAL producer: one raw spike has an IIR tail across several
         * fresh publications. Repeated cached polls cannot confirm that tail. */
        for (int i = 0; i < 40; ++i) {
            smk__wheel(1, 735);
            for (int poll = 0; poll < 64; ++poll) smk_wheels();
            audio(); CHECK(engine_bass_perf(voice, 2, 1) == -1);
            for (int sample = 0; sample < 80; ++sample) {
                smk__wheel(1, 615);
                for (int poll = 0; poll < 8; ++poll) smk_wheels();
                audio(); CHECK(engine_bass_perf(voice, 2, 1) == -1);
                CHECK(mod_lane(voice, parameter) < 0);
            }
        }
        CHECK(!memcmp(&before, &proj.sound, sizeof before));
        CHECK(engine_bass_perf(voice, 0, 1) == 64);
        CHECK(mod_lane(voice, bank ? ORIGINAL_MIAMI_PITCH_BEND : ORIGINAL_CHICAGO_PITCH_BEND) < 0);
        ui.rec = 0; seq_stop(&seq); audio();
        /* The keyboard knob takes ownership; noise cannot reclaim it. */
        pref_t r = PR(R_ENG, T_303, voice, parameter);
        pref_set(r, 51); audio();
        isolated_spikes(voice, parameter, -1);
        CHECK(pref_get(r) == 51 && engine_bass_perf(voice, 2, 0) == 51);
        /* Deliberate movement and offscreen release keep the original owner. */
        wheel_sample(550, 550);
        /* The IIR's integer tail settles at 618, inside the rest deadband.
         * (618-550-12)*127/(618-485-12) = 58, with integer truncation. */
        CHECK(engine_bass_perf(voice, 2, 1) == 58);
        ui.part = PART_303A + (voice ^ 1); build_pages();
        wheel_sample(550, 615);
        CHECK(engine_bass_perf(voice, 2, 1) == 0);
        CHECK(engine_bass_perf(voice ^ 1, 2, 1) == -1);
    }
    printf("PASS %u B4 checks voice=%d bank=%d REC=%d expand=%d\n",
           checks, voice, bank, record, expand);
    return 0;
}
