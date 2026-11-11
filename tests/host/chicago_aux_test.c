/* SPDX-License-Identifier: GPL-3.0-only */
/* Host proof: Chicago pitch bend (±2 st live, no TUNE rewrite) + damper
 * (sustain defers note-off). Indices fill NPARAMS_MAX after SEND FX. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "bass303.h"
#include "chicago_aux.h"
#include "chicago_fx.h"

static int fails;
#define CHECK(c, ...)                                                                                                  \
    do {                                                                                                               \
        if (!(c)) {                                                                                                    \
            printf("FAIL " __VA_ARGS__);                                                                               \
            printf("\n");                                                                                              \
            fails++;                                                                                                   \
        }                                                                                                              \
    } while (0)

static float buf[8192];

static double period_est(const float *x, int a, int b)
{
    int i, z = 0, first = -1, last = -1;
    for (i = a + 1; i < b; i++) {
        if ((x[i - 1] < 0.0f && x[i] >= 0.0f) || (x[i - 1] >= 0.0f && x[i] < 0.0f)) {
            if (first < 0)
                first = i;
            last = i;
            z++;
        }
    }
    if (z < 2 || first < 0 || last <= first)
        return 0;
    return (double)(last - first) * 2.0 / (double)(z - 1);
}

static void test_indices(void)
{
    CHECK(CHI_FX_END == 30, "SEND FX ends at 30");
    CHECK(CHI_PBEND == 30 && CHI_DAMPER == 31, "aux pots at 30..31");
    CHECK(CHI_AUX_END == 32, "fills NPARAMS_MAX 32");
    CHECK(BASS303_NPARAMS == 23, "bass303 DSP pots stay 0..22");
    CHECK(CHI_AUX_PARAMS[0].def == 64 && !strcmp(CHI_AUX_PARAMS[0].name, "PBend"), "PBend def centre");
    CHECK(CHI_AUX_PARAMS[1].def == 0 && !strcmp(CHI_AUX_PARAMS[1].name, "Damp"), "Damp def Off");
    CHECK(chi_damper_on(63) == 0 && chi_damper_on(64) == 1, "damper thresh 64");
    CHECK(fabsf(chi_pbend_semitones(64)) < 1e-6f, "centre = 0 st");
    CHECK(fabsf(chi_pbend_semitones(0) + 2.0f) < 1e-5f, "0 = −2 st");
    CHECK(fabsf(chi_pbend_semitones(127) - 2.0f) < 0.05f, "127 ≈ +2 st");
    CHECK(chi_pbend_from_wheel(0) == 64, "wheel rest → centre pot");
    CHECK(chi_pbend_from_wheel(-63) == 1, "wheel full down");
    CHECK(chi_pbend_from_wheel(63) == 127, "wheel full up");
    CHECK(chi_pbend_from_wheel(-100) == 1 && chi_pbend_from_wheel(100) == 127, "wheel clamps");
}

static void test_factory_decode(void)
{
    uint8_t in[2] = {128, 0}, out[2]; /* AU 0.5 → ~64, damper 0 */
    chi_aux_decode_factory(in, out);
    CHECK(out[0] == 64, "factory centre bend → 64");
    CHECK(out[1] == 0, "factory damper 0 → 0");
    in[0] = 0;
    in[1] = 255;
    chi_aux_decode_factory(in, out);
    CHECK(out[0] == 0, "factory bend 0");
    CHECK(out[1] == 127, "factory damper full → 127");
}

static void test_bend_live(void)
{
    bass303_t b;
    double p0, p1, ratio;
    bass303_init(&b);
    bass303_set(&b, BASS303_WAVE, 0);
    bass303_set(&b, BASS303_CUTOFF, 127);
    bass303_set(&b, BASS303_RESO, 0);
    bass303_set(&b, BASS303_ENVMOD, 0);
    bass303_set(&b, BASS303_VOLUME, 100);
    bass303_set(&b, BASS303_TUNE, 64);
    CHECK(b.pot[BASS303_TUNE] == 64, "TUNE stays 64 before bend");
    bass303_note_on(&b, 69, 0, 0); /* A4 */
    bass303_render(&b, buf, 4096);
    p0 = period_est(buf, 512, 4096);
    CHECK(p0 > 0, "got period at centre");
    bass303_set_bend(&b, 127); /* +2 st */
    CHECK(b.pot[BASS303_TUNE] == 64, "bend must not rewrite TUNE pot");
    bass303_render(&b, buf, 4096);
    p1 = period_est(buf, 512, 4096);
    CHECK(p1 > 0, "got period after bend");
    ratio = p0 / p1; /* higher pitch → shorter period */
    CHECK(ratio > 1.08 && ratio < 1.16, "≈ +2 st period ratio (got %.4f)", ratio);
    bass303_set_bend(&b, 64);
    bass303_note_off(&b);
}

static void test_bend_on_note(void)
{
    bass303_t b;
    double p;
    bass303_init(&b);
    bass303_set(&b, BASS303_WAVE, 0);
    bass303_set(&b, BASS303_CUTOFF, 127);
    bass303_set(&b, BASS303_RESO, 0);
    bass303_set(&b, BASS303_ENVMOD, 0);
    bass303_set(&b, BASS303_VOLUME, 100);
    bass303_set_bend(&b, 0); /* −2 st before note */
    bass303_note_on(&b, 69, 0, 0);
    bass303_render(&b, buf, 4096);
    p = period_est(buf, 512, 4096);
    /* A4 @ −2 st ≈ 440/2^(2/12) ≈ 391.9 Hz → period ≈ 112.5 samples */
    CHECK(p > 105 && p < 120, "pre-bend note period ≈ −2 st (got %.2f)", p);
    bass303_note_off(&b);
}

static void test_gate_without_engine_damper(void)
{
    /* Damper policy lives in engine (live paths). DSP note_off always releases.
     * Engine invariant (Bugbot #342): pattern s_bon/s_boff and arp steps clear
     * chi_damp_latched so pedal-up cannot cut a sequencer gate. */
    bass303_t b;
    bass303_init(&b);
    bass303_note_on(&b, 60, 0, 0);
    CHECK(b.gate == 1, "gate on");
    bass303_note_off(&b);
    CHECK(b.gate == 0, "DSP note_off clears gate (engine defers separately)");
}

int main(void)
{
    test_indices();
    test_factory_decode();
    test_bend_live();
    test_bend_on_note();
    test_gate_without_engine_damper();
    if (fails) {
        printf("%d FAIL chicago-aux\n", fails);
        return 1;
    }
    printf("ok chicago-aux (PBend ±2 st live, Damp surface, no TUNE rewrite)\n");
    return 0;
}
