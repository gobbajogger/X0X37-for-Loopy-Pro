/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago AmpMod Env/Gate on native303: Gate holds VCA while the note is down; Env keeps
 * the Open303 amp path. Independent of EnvMod, FEnv, and velocity→cutoff. */
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

static void render_held(bass303_t *b, int mode, float *out, int n)
{
    int pos = 0;
    bass303_init(b);
    bass303_set(b, BASS303_CUTOFF, 64);
    bass303_set(b, BASS303_RESO, 0);
    bass303_set(b, BASS303_ENVMOD, 0);
    bass303_set(b, BASS303_FENV, 0);
    bass303_set(b, BASS303_AMPMODE, mode);
    bass303_set(b, BASS303_VOLUME, 96);
    bass303_set(b, BASS303_DRIVE, 0);
    bass303_note_on_velocity(b, 48, 0, 0, 100);
    while (pos < n) {
        int m = n - pos > 128 ? 128 : n - pos;
        bass303_render(b, out + pos, m);
        pos += m;
    }
}

static void test_param_surface(void)
{
    const x0x_param_t *p = bass303_param(BASS303_AMPMODE);
    CHECK(bass303_nparams() == BASS303_NPARAMS, "nparams includes AmpMod");
    CHECK(BASS303_AMPMODE == 16, "AmpMod index is 16 (after FEnv); sibling param ports use 17+");
    CHECK(p && p->max == 1 && p->def == BASS303_AMP_ENV, "AmpMod default Env");
    CHECK(p && p->names && !strcmp(p->names[0], "Env") && !strcmp(p->names[1], "Gate"),
          "AmpMod labels Env/Gate");
    CHECK(p && strcmp(p->name, "EnvMod") != 0 && strcmp(p->name, "FEnv") != 0,
          "AmpMod name is not EnvMod/FEnv");
}

static void test_gate_holds_env_falls(void)
{
    bass303_t env, gate;
    double e0, e1, g0, g1, env_drop_db, gate_drop_db;
    render_held(&env, BASS303_AMP_ENV, buf, 44100);
    CHECK(env.amp_y < 0.85f, "Env amp_y decays while held (got %g)", (double)env.amp_y);
    e0 = rms_range(buf, 2205, 4410);   /* ~50-100 ms */
    e1 = rms_range(buf, 17640, 19845); /* ~400-450 ms */
    render_held(&gate, BASS303_AMP_GATE, buf, 44100);
    CHECK(fabsf(gate.amp_y - 1.0f) < 1e-6f, "Gate amp_y holds at 1 while held (got %g)", (double)gate.amp_y);
    g0 = rms_range(buf, 2205, 4410);
    g1 = rms_range(buf, 17640, 19845);
    CHECK(e0 > 1e-6 && e1 > 1e-6 && g0 > 1e-6 && g1 > 1e-6, "both modes produce audio");
    env_drop_db = 20.0 * log10(e1 / e0);
    gate_drop_db = 20.0 * log10(g1 / g0);
    /* Filter EG can still move RMS under Gate (Chicago: ADSR still hits the filter). VCA hold is
     * the AmpMod contract; Gate RMS must fall less than Env. */
    CHECK(env_drop_db < -0.5, "Env output falls while held (got %.3f dB)", env_drop_db);
    CHECK(gate_drop_db > env_drop_db + 0.4, "Gate output falls less than Env (gate %.3f env %.3f)",
          gate_drop_db, env_drop_db);
}

static void test_gate_releases(void)
{
    bass303_t b;
    double mid, tail;
    int pos = 0;
    bass303_init(&b);
    bass303_set(&b, BASS303_AMPMODE, BASS303_AMP_GATE);
    bass303_set(&b, BASS303_VOLUME, 96);
    bass303_set(&b, BASS303_DRIVE, 0);
    bass303_note_on_velocity(&b, 48, 0, 0, 100);
    while (pos < 22050) {
        int m = 128;
        if (pos + m > 22050)
            m = 22050 - pos;
        bass303_render(&b, buf + pos, m);
        pos += m;
    }
    bass303_note_off(&b);
    while (pos < 44100) {
        int m = 128;
        if (pos + m > 44100)
            m = 44100 - pos;
        bass303_render(&b, buf + pos, m);
        pos += m;
    }
    mid = rms_range(buf, 11025, 16538);
    tail = rms_range(buf, 33075, 44100);
    CHECK(mid > 1e-4, "Gate sounds while held");
    CHECK(tail < mid * 0.05, "Gate releases after note-off");
}

static void test_env_matches_default_init(void)
{
    bass303_t a, b;
    float xa[256], xb[256];
    int i;
    bass303_init(&a);
    bass303_init(&b);
    CHECK(bass303_get(&a, BASS303_AMPMODE) == BASS303_AMP_ENV, "init AmpMod is Env");
    bass303_set(&b, BASS303_AMPMODE, BASS303_AMP_ENV);
    bass303_note_on_velocity(&a, 48, 0, 0, 100);
    bass303_note_on_velocity(&b, 48, 0, 0, 100);
    bass303_render(&a, xa, 256);
    bass303_render(&b, xb, 256);
    for (i = 0; i < 256; i++)
        if (xa[i] != xb[i])
            break;
    CHECK(i == 256, "explicit Env matches default AmpMod path bit-exact");
}

int main(void)
{
    fails = 0;
    test_param_surface();
    test_gate_holds_env_falls();
    test_gate_releases();
    test_env_matches_default_init();
    if (fails) {
        printf("%d FAIL\n", fails);
        return 1;
    }
    printf("bass303_ampmode: OK\n");
    return 0;
}
