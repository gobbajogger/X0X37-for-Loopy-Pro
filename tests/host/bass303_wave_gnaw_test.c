/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago OSC Wave continuous morph + VCF Gnaw on native303. Wave 0=saw .. 127=square;
 * old discrete square (1) migrates to 127 in project load. Gnaw is pre-filter grit. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "bass303.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL " __VA_ARGS__); printf("\n"); fails++; } } while (0)

static float buf[8192];

static double rms(const float *x, int n)
{
    double s = 0;
    int i;
    for (i = 0; i < n; i++)
        s += (double)x[i] * (double)x[i];
    return sqrt(s / (double)n);
}

static void render_wave(bass303_t *b, int wave, float *out, int n)
{
    int pos = 0;
    bass303_init(b);
    bass303_set(b, BASS303_CUTOFF, 90);
    bass303_set(b, BASS303_RESO, 0);
    bass303_set(b, BASS303_ENVMOD, 0);
    bass303_set(b, BASS303_WAVE, wave);
    bass303_set(b, BASS303_GNAW, 0);
    bass303_set(b, BASS303_VOLUME, 96);
    bass303_set(b, BASS303_DRIVE, 0);
    bass303_set(b, BASS303_AMPMODE, BASS303_AMP_GATE);
    bass303_note_on_velocity(b, 48, 0, 0, 100);
    while (pos < n) {
        int m = n - pos > 128 ? 128 : n - pos;
        bass303_render(b, out + pos, m);
        pos += m;
    }
}

static void test_surface(void)
{
    const x0x_param_t *w = bass303_param(BASS303_WAVE);
    const x0x_param_t *g = bass303_param(BASS303_GNAW);
    CHECK(BASS303_GNAW == 17, "Gnaw index is 17 (after AmpMod 16)");
    CHECK(w && w->max == 127 && w->def == 0 && !w->names, "Wave is continuous 0..127 saw→square");
    CHECK(g && g->max == 127 && g->def == 0 && !strcmp(g->name, "Gnaw"), "Gnaw pot 0..127 default 0");
    CHECK(bass303_nparams() == BASS303_NPARAMS, "nparams includes Gnaw");
}

static void test_wave_endpoints_differ(void)
{
    bass303_t saw, sqr, mid;
    double rs, rq, rm;
    render_wave(&saw, 0, buf, 4096);
    rs = rms(buf + 512, 3072);
    CHECK(fabsf(saw.wave_mix) < 1e-6f, "Wave 0 → wave_mix 0 (got %g)", (double)saw.wave_mix);
    render_wave(&sqr, 127, buf, 4096);
    rq = rms(buf + 512, 3072);
    CHECK(fabsf(sqr.wave_mix - 1.0f) < 1e-5f, "Wave 127 → wave_mix 1 (got %g)", (double)sqr.wave_mix);
    render_wave(&mid, 64, buf, 4096);
    rm = rms(buf + 512, 3072);
    CHECK(fabsf(mid.wave_mix - (64.0f / 127.0f)) < 1e-5f, "Wave 64 morph fraction");
    CHECK(rs > 1e-5 && rq > 1e-5 && rm > 1e-5, "saw/square/mid produce audio");
    /* Morph mid should not collapse to silence relative to endpoints. */
    CHECK(rm > 0.25 * (rs < rq ? rs : rq), "mid morph has usable level (mid %g ends %g/%g)", rm, rs, rq);
}

static void test_gnaw_changes_level(void)
{
    bass303_t clean, dirty;
    double rc, rd;
    int pos;
    bass303_init(&clean);
    bass303_set(&clean, BASS303_CUTOFF, 100);
    bass303_set(&clean, BASS303_RESO, 40);
    bass303_set(&clean, BASS303_WAVE, 0);
    bass303_set(&clean, BASS303_GNAW, 0);
    bass303_set(&clean, BASS303_VOLUME, 96);
    bass303_set(&clean, BASS303_DRIVE, 0);
    bass303_set(&clean, BASS303_AMPMODE, BASS303_AMP_GATE);
    bass303_note_on_velocity(&clean, 36, 0, 0, 100);
    for (pos = 0; pos < 4096; ) {
        int m = 4096 - pos > 128 ? 128 : 4096 - pos;
        bass303_render(&clean, buf + pos, m);
        pos += m;
    }
    rc = rms(buf + 512, 3072);

    bass303_init(&dirty);
    bass303_set(&dirty, BASS303_CUTOFF, 100);
    bass303_set(&dirty, BASS303_RESO, 40);
    bass303_set(&dirty, BASS303_WAVE, 0);
    bass303_set(&dirty, BASS303_GNAW, 100);
    bass303_set(&dirty, BASS303_VOLUME, 96);
    bass303_set(&dirty, BASS303_DRIVE, 0);
    bass303_set(&dirty, BASS303_AMPMODE, BASS303_AMP_GATE);
    bass303_note_on_velocity(&dirty, 36, 0, 0, 100);
    for (pos = 0; pos < 4096; ) {
        int m = 4096 - pos > 128 ? 128 : 4096 - pos;
        bass303_render(&dirty, buf + pos, m);
        pos += m;
    }
    rd = rms(buf + 512, 3072);
    CHECK(fabsf(dirty.gnaw - (100.0f / 127.0f)) < 1e-5f, "Gnaw 100 stores fraction");
    CHECK(rc > 1e-5 && rd > 1e-5, "clean and gnaw produce audio");
    /* Pre-filter tanh grit changes spectrum/level vs clean; must not match bit-exact. */
    CHECK(fabs(rd - rc) / rc > 0.01, "Gnaw changes output vs clean (clean %g gnaw %g)", rc, rd);
}

int main(void)
{
    test_surface();
    test_wave_endpoints_differ();
    test_gnaw_changes_level();
    if (fails) {
        printf("bass303_wave_gnaw: %d FAIL\n", fails);
        return 1;
    }
    printf("bass303_wave_gnaw: OK\n");
    return 0;
}
