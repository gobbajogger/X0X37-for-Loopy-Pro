/* SPDX-License-Identifier: GPL-3.0-only */
/* Host proof: Chicago EQ insert tilts spectrum vs dry native303. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "bass303.h"
#include "chicago_eq.h"
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
static float dry[8192];

static void render_note(bass303_t *b, float *out, int n)
{
    int pos = 0;
    bass303_init(b);
    bass303_set(b, BASS303_CUTOFF, 100);
    bass303_set(b, BASS303_RESO, 40);
    bass303_set(b, BASS303_ENVMOD, 0);
    bass303_set(b, BASS303_WAVE, 0);
    bass303_set(b, BASS303_GNAW, 0);
    bass303_set(b, BASS303_BITE, 0);
    bass303_set(b, BASS303_VOLUME, 100);
    bass303_set(b, BASS303_DRIVE, 0);
    bass303_set(b, BASS303_AMPMODE, BASS303_AMP_GATE);
    bass303_note_on_velocity(b, 48, 0, 0, 100);
    while (pos < n) {
        int m = n - pos > 128 ? 128 : n - pos;
        bass303_render(b, out + pos, m);
        pos += m;
    }
}

static double rms_delta(const float *a, const float *b, int n)
{
    double s = 0;
    int i;
    for (i = 0; i < n; i++) {
        double d = (double)a[i] - (double)b[i];
        s += d * d;
    }
    return sqrt(s / (double)n);
}

/* Crude highband energy: sum |x[i]-x[i-1]| (bright EQ raises this). */
static double edge_energy(const float *x, int n)
{
    double s = 0;
    int i;
    for (i = 1; i < n; i++)
        s += fabs((double)x[i] - (double)x[i - 1]);
    return s / (double)n;
}

static void test_supported_flags(void)
{
    CHECK(chi_fx_supported(CHI_FX_EQ), "EQ supported");
    CHECK(chi_fx_is_insert(CHI_FX_EQ), "EQ is insert");
    CHECK(chi_fx_is_eq_insert(CHI_FX_EQ), "EQ eq-insert helper");
    CHECK(!chi_fx_is_not_yet(CHI_FX_EQ), "EQ not NOT_YET");
    CHECK(strstr(CHI_FX_TYPE_N[CHI_FX_EQ], "—") == 0, "EQ label has no em-dash");
}

static void test_map_clears_sends(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_EQ, 0, 127, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "EQ insert clears sends");
    CHECK(!(o.touch & CHI_FX_W_COMP), "EQ does not set Comp");
    CHECK(!(o.touch & CHI_FX_W_DIST), "EQ does not set Dist");
    CHECK(!(o.touch & CHI_FX_W_MX_DLY), "EQ does not set Mix delay");
}

static void test_tilt_and_wet0(void)
{
    bass303_t b;
    chi_eq_t eq;
    int n = 4096, skip = 512;
    double dark_edge, bright_edge, delta;
    render_note(&b, dry, n);

    memcpy(buf, dry, (size_t)n * sizeof(float));
    chi_eq_init(&eq);
    chi_eq_set(&eq, 0, 127, BASS303_SR); /* dark + full wet */
    chi_eq_process(&eq, buf + skip, n - skip);
    dark_edge = edge_energy(buf + skip, n - skip);
    delta = rms_delta(dry + skip, buf + skip, n - skip);
    CHECK(delta > 1e-4, "dark EQ changes waveform (rms delta %g)", delta);

    memcpy(buf, dry, (size_t)n * sizeof(float));
    chi_eq_init(&eq);
    chi_eq_set(&eq, 127, 127, BASS303_SR); /* bright + full wet */
    chi_eq_process(&eq, buf + skip, n - skip);
    bright_edge = edge_energy(buf + skip, n - skip);
    CHECK(bright_edge > dark_edge * 1.05, "bright EQ has more edge energy (dark %g bright %g)", dark_edge,
          bright_edge);

    memcpy(buf, dry, (size_t)n * sizeof(float));
    chi_eq_init(&eq);
    chi_eq_process(&eq, buf + skip, n - skip);
    CHECK(memcmp(buf + skip, dry + skip, (size_t)(n - skip) * sizeof(float)) == 0, "unset EQ leaves PCM unchanged");
}

int main(void)
{
    test_supported_flags();
    test_map_clears_sends();
    test_tilt_and_wet0();
    if (fails) {
        printf("%d FAIL(s)\n", fails);
        return 1;
    }
    printf("PASS chicago_eq\n");
    return 0;
}
