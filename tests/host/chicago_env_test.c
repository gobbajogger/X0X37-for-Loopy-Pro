/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago AU Env → filter EG dual-drive (map helper + Gate DSP path). */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "bass303.h"
#include "chicago_env.h"

static int fails;
#define CHECK(c, ...)                                                                                                  \
    do {                                                                                                               \
        if (!(c)) {                                                                                                    \
            printf("FAIL " __VA_ARGS__);                                                                               \
            printf("\n");                                                                                              \
            fails++;                                                                                                   \
        }                                                                                                              \
    } while (0)

static float buf[48000];

static float rms(const float *x, int n)
{
    double s = 0.0;
    int i;
    for (i = 0; i < n; i++)
        s += (double)x[i] * (double)x[i];
    return (float)sqrt(s / (double)n);
}

static void test_factory_mirror(void)
{
    uint8_t voice[BASS303_NPARAMS];
    uint8_t env[4] = {102, 90, 56, 8}; /* Acid Lab 1–ish Env bytes */
    int i;
    for (i = 0; i < BASS303_NPARAMS; i++)
        voice[i] = 0;
    voice[BASS303_DECAY] = 64;
    voice[BASS303_FENV] = 0;
    chi_env_apply_factory_voice(voice, env);
    CHECK(voice[BASS303_AATTACK] == chi_env_pot_from_byte(102, 127), "AAtt from Env Attack");
    CHECK(voice[BASS303_ADECAY] == chi_env_pot_from_byte(90, 127), "ADec from Env Decay");
    CHECK(voice[BASS303_ASUSTAIN] == chi_env_pot_from_byte(56, 127), "ASus from Env Sustain");
    CHECK(voice[BASS303_ARELEASE] == chi_env_pot_from_byte(8, 127), "ARel from Env Release");
    CHECK(voice[BASS303_FATTACK] == voice[BASS303_AATTACK], "FAttack mirrors AAtt");
    CHECK(voice[BASS303_DECAY] == voice[BASS303_ADECAY], "filter Decay mirrors ADec");
    CHECK(voice[BASS303_FSUSTAIN] == voice[BASS303_ASUSTAIN], "FSustain mirrors ASus");
    CHECK(voice[BASS303_FRELEASE] == voice[BASS303_ARELEASE], "FRelease mirrors ARel");
    CHECK(voice[BASS303_FENV] == 1, "FEnv ADSR");
}

/* Gate + Env Sustain: filter must hold sustain (not 303 decay-to-zero). */
static void test_gate_filter_sustain(void)
{
    bass303_t b;
    float early, late;
    bass303_init(&b);
    bass303_set(&b, BASS303_WAVE, 0);
    bass303_set(&b, BASS303_CUTOFF, 0);
    bass303_set(&b, BASS303_RESO, 100);
    bass303_set(&b, BASS303_ENVMOD, 110);
    bass303_set(&b, BASS303_VOLUME, 100);
    bass303_set(&b, BASS303_AMPMODE, BASS303_AMP_GATE);
    bass303_set(&b, BASS303_AATTACK, 0);
    bass303_set(&b, BASS303_ADECAY, 40);   /* ~filter decay time from pot */
    bass303_set(&b, BASS303_ASUSTAIN, 80); /* hold filter open */
    bass303_set(&b, BASS303_ARELEASE, 0);
    /* Leave FEnv=0 / FSustain=0 — Gate DSP must still use amp Env for filter. */
    CHECK(b.pot[BASS303_FENV] == 0, "FEnv stays 303 until Gate path engages");
    bass303_note_on(&b, 48, 0, 0);
    bass303_render(&b, buf, 24000);
    early = rms(buf + 2000, 2000);
    late = rms(buf + 18000, 4000);
    CHECK(early > 1e-4f, "Gate note is audible early (%.6g)", early);
    CHECK(late > early * 0.25f, "Gate+sustain keeps late energy (early=%.6g late=%.6g)", early, late);
    bass303_note_off(&b);
    bass303_render(&b, buf, 8000);
}

/* Env mode + explicit FEnv ADSR sustain (AU map dual-drive). */
static void test_fenv_adsr_sustain(void)
{
    bass303_t b;
    float early, late;
    bass303_init(&b);
    bass303_set(&b, BASS303_WAVE, 0);
    bass303_set(&b, BASS303_CUTOFF, 0);
    bass303_set(&b, BASS303_RESO, 90);
    bass303_set(&b, BASS303_ENVMOD, 120);
    bass303_set(&b, BASS303_VOLUME, 100);
    bass303_set(&b, BASS303_AMPMODE, BASS303_AMP_ENV);
    bass303_set(&b, BASS303_AATTACK, 0);
    bass303_set(&b, BASS303_ADECAY, 30);
    bass303_set(&b, BASS303_ASUSTAIN, 90);
    bass303_set(&b, BASS303_ARELEASE, 20);
    bass303_set(&b, BASS303_FATTACK, 0);
    bass303_set(&b, BASS303_DECAY, 30);
    bass303_set(&b, BASS303_FSUSTAIN, 90);
    bass303_set(&b, BASS303_FRELEASE, 20);
    bass303_set(&b, BASS303_FENV, 1);
    bass303_note_on(&b, 48, 0, 0);
    bass303_render(&b, buf, 24000);
    early = rms(buf + 2000, 2000);
    late = rms(buf + 18000, 4000);
    CHECK(early > 1e-4f, "FEnv ADSR audible early");
    CHECK(late > early * 0.2f, "FEnv sustain keeps late energy (early=%.6g late=%.6g)", early, late);
}

int main(void)
{
    test_factory_mirror();
    test_gate_filter_sustain();
    test_fenv_adsr_sustain();
    if (fails) {
        printf("%d FAIL chicago_env\n", fails);
        return 1;
    }
    puts("PASS chicago_env");
    return 0;
}
