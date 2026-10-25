/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago amp Env Attack/Decay/Sustain/Release on native303. Defaults preserve Open303
 * Env decay-while-held; Gate mode still holds VCA; AmpMod index 16 untouched. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "bass303.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL " __VA_ARGS__); printf("\n"); fails++; } } while (0)

static float buf[44100];

static double rms_range(const float *x, int a, int b)
{
    double s = 0;
    int i, n = b - a;
    if (n <= 0)
        return 0;
    for (i = a; i < b; i++)
        s += (double)x[i] * (double)x[i];
    return sqrt(s / (double)n);
}

static void test_surface(void)
{
    const x0x_param_t *a = bass303_param(BASS303_AATTACK);
    const x0x_param_t *d = bass303_param(BASS303_ADECAY);
    const x0x_param_t *s = bass303_param(BASS303_ASUSTAIN);
    const x0x_param_t *r = bass303_param(BASS303_ARELEASE);
    CHECK(BASS303_AMPMODE == 16, "AmpMod stays at 16");
    CHECK(BASS303_GNAW == 17, "Gnaw stays at 17");
    CHECK(BASS303_BITE == 18, "Bite index is 18");
    CHECK(BASS303_AATTACK == 19 && BASS303_ADECAY == 20 && BASS303_ASUSTAIN == 21 &&
          BASS303_ARELEASE == 22, "amp ADSR indices 19..22");
    CHECK(bass303_nparams() == 23, "nparams includes Bite + amp ADSR");
    CHECK(a && a->def == 0 && !strcmp(a->name, "AAtt"), "AAtt default 0");
    CHECK(d && d->def == 64 && !strcmp(d->name, "ADec"), "ADec default 64 (~1230 ms)");
    CHECK(s && s->def == 0 && !strcmp(s->name, "ASus"), "ASus default 0");
    CHECK(r && r->def == 0 && !strcmp(r->name, "ARel"), "ARel default 0 (stock release)");
}

static void test_default_env_still_decays(void)
{
    bass303_t b;
    double e0, e1;
    int pos = 0;
    bass303_init(&b);
    CHECK(bass303_get(&b, BASS303_AATTACK) == 0, "init AAtt 0");
    CHECK(bass303_get(&b, BASS303_ADECAY) == 64, "init ADec 64");
    CHECK(bass303_get(&b, BASS303_ASUSTAIN) == 0, "init ASus 0");
    CHECK(bass303_get(&b, BASS303_AMPMODE) == BASS303_AMP_ENV, "init AmpMod Env");
    bass303_set(&b, BASS303_VOLUME, 96);
    bass303_set(&b, BASS303_DRIVE, 0);
    bass303_set(&b, BASS303_ENVMOD, 0);
    bass303_note_on_velocity(&b, 48, 0, 0, 100);
    while (pos < 44100) {
        int m = 128;
        if (pos + m > 44100)
            m = 44100 - pos;
        bass303_render(&b, buf + pos, m);
        pos += m;
    }
    e0 = rms_range(buf, 2205, 4410);
    e1 = rms_range(buf, 17640, 19845);
    CHECK(b.amp_y < 0.85f, "default Env amp_y decays (got %g)", (double)b.amp_y);
    CHECK(e0 > 1e-6 && e1 > 1e-6, "default Env produces audio");
    CHECK(20.0 * log10(e1 / e0) < -0.5, "default Env output falls while held");
}

static void test_sustain_holds(void)
{
    bass303_t b;
    int pos = 0;
    bass303_init(&b);
    bass303_set(&b, BASS303_AMPMODE, BASS303_AMP_ENV);
    bass303_set(&b, BASS303_AATTACK, 0);
    bass303_set(&b, BASS303_ADECAY, 5);   /* ~96 ms → settle well within 2 s */
    bass303_set(&b, BASS303_ASUSTAIN, 64);
    bass303_set(&b, BASS303_VOLUME, 96);
    bass303_set(&b, BASS303_DRIVE, 0);
    bass303_set(&b, BASS303_ENVMOD, 0);
    bass303_note_on_velocity(&b, 48, 0, 0, 100);
    while (pos < 88200) {
        float tmp[128];
        int m = 128;
        if (pos + m > 88200)
            m = 88200 - pos;
        bass303_render(&b, tmp, m);
        pos += m;
    }
    CHECK(b.amp_stage == BASS303_AMP_SUS || fabsf(b.amp_y - b.asustain) < 0.02f,
          "reached sustain (stage=%d amp_y=%g sus=%g)", b.amp_stage,
          (double)b.amp_y, (double)b.asustain);
    CHECK(fabsf(b.amp_y - b.asustain) < 0.02f, "amp_y near sustain (got %g want %g)",
          (double)b.amp_y, (double)b.asustain);
}

static void test_attack_delays_peak(void)
{
    bass303_t fast, slow;
    double rf, rs;
    int pos;
    bass303_init(&fast);
    bass303_set(&fast, BASS303_AATTACK, 0);
    bass303_set(&fast, BASS303_ASUSTAIN, 127);
    bass303_set(&fast, BASS303_ADECAY, 10);
    bass303_set(&fast, BASS303_VOLUME, 96);
    bass303_set(&fast, BASS303_DRIVE, 0);
    bass303_set(&fast, BASS303_ENVMOD, 0);
    bass303_note_on_velocity(&fast, 48, 0, 0, 100);
    for (pos = 0; pos < 2048; ) {
        int m = 128;
        bass303_render(&fast, buf + pos, m);
        pos += m;
    }
    rf = rms_range(buf, 0, 512);

    bass303_init(&slow);
    bass303_set(&slow, BASS303_AATTACK, 100);
    bass303_set(&slow, BASS303_ASUSTAIN, 127);
    bass303_set(&slow, BASS303_ADECAY, 10);
    bass303_set(&slow, BASS303_VOLUME, 96);
    bass303_set(&slow, BASS303_DRIVE, 0);
    bass303_set(&slow, BASS303_ENVMOD, 0);
    bass303_note_on_velocity(&slow, 48, 0, 0, 100);
    for (pos = 0; pos < 2048; ) {
        int m = 128;
        bass303_render(&slow, buf + pos, m);
        pos += m;
    }
    rs = rms_range(buf, 0, 512);
    CHECK(rf > 1e-5, "instant attack has early energy");
    CHECK(rs < rf * 0.85, "slow attack quieter in first 512 samples (slow %g fast %g)", rs, rf);
}

static void test_gate_unchanged(void)
{
    bass303_t b;
    int pos = 0;
    bass303_init(&b);
    bass303_set(&b, BASS303_AMPMODE, BASS303_AMP_GATE);
    bass303_set(&b, BASS303_AATTACK, 80);
    bass303_set(&b, BASS303_ASUSTAIN, 40);
    bass303_set(&b, BASS303_VOLUME, 96);
    bass303_set(&b, BASS303_DRIVE, 0);
    bass303_note_on_velocity(&b, 48, 0, 0, 100);
    while (pos < 22050) {
        int m = 128;
        bass303_render(&b, buf + pos, m);
        pos += m;
    }
    CHECK(fabsf(b.amp_y - 1.0f) < 1e-6f, "Gate still holds at 1 despite amp ADSR pots");
}

int main(void)
{
    test_surface();
    test_default_env_still_decays();
    test_sustain_holds();
    test_attack_delays_peak();
    test_gate_unchanged();
    if (fails) {
        printf("%d FAIL(s)\n", fails);
        return 1;
    }
    puts("PASS bass303_amp_adsr");
    return 0;
}
