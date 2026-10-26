/* SPDX-License-Identifier: GPL-3.0-only */
/* Host proof: Chicago Cho/Flg insert adds modulated delay colour vs dry native303. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "bass303.h"
#include "chicago_cho.h"
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
    bass303_set(b, BASS303_WAVE, 1); /* square — Detuned Square class */
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

static void test_supported_flags(void)
{
    CHECK(chi_fx_supported(CHI_FX_CHO), "Cho supported");
    CHECK(chi_fx_is_insert(CHI_FX_CHO), "Cho is insert");
    CHECK(chi_fx_is_cho_insert(CHI_FX_CHO), "Cho cho-insert helper");
    CHECK(!chi_fx_is_not_yet(CHI_FX_CHO), "Cho not NOT_YET");
    CHECK(strstr(CHI_FX_TYPE_N[CHI_FX_CHO], "—") == 0, "Cho label has no em-dash");
}

static void test_map_clears_sends(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_CHO, 47, 16, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "Cho insert clears sends");
    CHECK(!(o.touch & CHI_FX_W_MX_REV), "Cho does not set Mix reverb");
    CHECK(!(o.touch & CHI_FX_W_MX_DLY), "Cho does not set Mix delay");
}

static void test_cho_changes_pcm(void)
{
    bass303_t b;
    chi_cho_t cho;
    int n = 4096, skip = 512;
    double delta, delta_hi;
    render_note(&b, dry, n);

    memcpy(buf, dry, (size_t)n * sizeof(float));
    chi_cho_init(&cho);
    chi_cho_set(&cho, 47, 16, BASS303_SR); /* Detuned Square-ish */
    chi_cho_process(&cho, buf + skip, n - skip);
    delta = rms_delta(dry + skip, buf + skip, n - skip);
    CHECK(delta > 1e-4, "Cho changes waveform (rms delta %g)", delta);

    memcpy(buf, dry, (size_t)n * sizeof(float));
    chi_cho_init(&cho);
    chi_cho_set(&cho, 127, 127, BASS303_SR);
    chi_cho_process(&cho, buf + skip, n - skip);
    delta_hi = rms_delta(dry + skip, buf + skip, n - skip);
    CHECK(delta_hi > delta * 1.02, "higher wet/depth moves PCM more (lo %g hi %g)", delta, delta_hi);

    memcpy(buf, dry, (size_t)n * sizeof(float));
    chi_cho_init(&cho);
    chi_cho_process(&cho, buf + skip, n - skip);
    CHECK(memcmp(buf + skip, dry + skip, (size_t)(n - skip) * sizeof(float)) == 0, "unset Cho leaves PCM unchanged");
}

int main(void)
{
    test_supported_flags();
    test_map_clears_sends();
    test_cho_changes_pcm();
    if (fails) {
        printf("%d FAIL(s)\n", fails);
        return 1;
    }
    printf("PASS chicago_cho\n");
    return 0;
}
