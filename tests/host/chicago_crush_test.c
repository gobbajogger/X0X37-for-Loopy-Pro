/* SPDX-License-Identifier: GPL-3.0-only */
/* Host proof: Chicago Crush/Decimator insert holds + quantises vs dry native303. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "bass303.h"
#include "chicago_fx.h"
#include "chicago_crush.h"

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

static int count_holds(const float *x, int n)
{
    int i, holds = 0;
    for (i = 1; i < n; i++) {
        if (x[i] == x[i - 1])
            holds++;
    }
    return holds;
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
    CHECK(chi_fx_supported(CHI_FX_DECIM), "Crush supported");
    CHECK(chi_fx_is_insert(CHI_FX_DECIM), "Crush is insert");
    CHECK(chi_fx_is_crush_insert(CHI_FX_DECIM), "Crush insert helper");
    CHECK(!chi_fx_is_not_yet(CHI_FX_DECIM), "Crush not NOT_YET");
    CHECK(strstr(CHI_FX_TYPE_N[CHI_FX_DECIM], "—") == 0, "Crush label has no em-dash");
}

static void test_map_clears_sends(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_DECIM, 70, 127, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "Crush map clears sends");
    CHECK(!(o.touch & CHI_FX_W_DIST), "Crush does not steal master Dist");
    CHECK(!(o.touch & CHI_FX_W_DRIVE), "Crush does not steal master Drive");
}

static void test_hold_and_delta(void)
{
    bass303_t b;
    chi_crush_t crush;
    int n = 4096, skip = 512;
    int holds_dry, holds_crush;
    double delta;
    render_note(&b, dry, n);
    memcpy(buf, dry, (size_t)n * sizeof(float));
    chi_crush_init(&crush);
    /* High Edit1 — insert is full-wet; sample-and-hold creates equal neighbours. */
    chi_crush_set(&crush, 110, 0);
    chi_crush_process(&crush, buf + skip, n - skip);
    holds_dry = count_holds(dry + skip, n - skip);
    holds_crush = count_holds(buf + skip, n - skip);
    CHECK(holds_crush > holds_dry + 200, "crush increases held-sample count (dry %d crush %d)", holds_dry,
          holds_crush);
    delta = rms_delta(dry + skip, buf + skip, n - skip);
    CHECK(delta > 1e-4, "crush insert changes waveform (rms delta %g)", delta);
    /* Unset insert (init wet=0) must be a no-op */
    memcpy(buf, dry, (size_t)n * sizeof(float));
    chi_crush_init(&crush);
    chi_crush_process(&crush, buf + skip, n - skip);
    CHECK(memcmp(buf + skip, dry + skip, (size_t)(n - skip) * sizeof(float)) == 0, "unset crush leaves PCM unchanged");
}

int main(void)
{
    test_supported_flags();
    test_map_clears_sends();
    test_hold_and_delta();
    if (fails) {
        printf("%d FAIL(s)\n", fails);
        return 1;
    }
    printf("PASS chicago_crush\n");
    return 0;
}
