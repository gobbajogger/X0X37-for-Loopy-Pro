/* SPDX-License-Identifier: GPL-3.0-only */
/* Host proof: Chicago Crush/Decimator insert quantizes + holds vs dry native303. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "bass303.h"
#include "chicago_crush.h"
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

static double rms(const float *x, int n)
{
    double s = 0;
    int i;
    for (i = 0; i < n; i++)
        s += (double)x[i] * (double)x[i];
    return sqrt(s / (double)n);
}

static int unique_levels(const float *x, int n, float eps)
{
    /* Count roughly distinct sample values (bitcrush collapses the set). */
    float seen[256];
    int nseen = 0, i, j;
    for (i = 0; i < n && nseen < 256; i++) {
        int hit = 0;
        for (j = 0; j < nseen; j++) {
            if (fabsf(x[i] - seen[j]) < eps) {
                hit = 1;
                break;
            }
        }
        if (!hit)
            seen[nseen++] = x[i];
    }
    return nseen;
}

static void test_supported_flags(void)
{
    CHECK(chi_fx_supported(CHI_FX_DECIM), "Crush supported");
    CHECK(chi_fx_is_insert(CHI_FX_DECIM), "Crush is insert");
    CHECK(chi_fx_is_crush_insert(CHI_FX_DECIM), "Crush crush-insert helper");
    CHECK(!chi_fx_is_not_yet(CHI_FX_DECIM), "Crush not NOT_YET");
    CHECK(strstr(CHI_FX_TYPE_N[CHI_FX_DECIM], "—") == 0, "Crush label has no em-dash");
}

static void test_map_clears_sends(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_DECIM, 80, 127, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "Crush map clears sends");
    CHECK(!(o.touch & CHI_FX_W_DIST), "Crush insert does not steal master Dist");
    CHECK(!(o.touch & CHI_FX_W_DRIVE), "Crush insert does not steal master Drive");
}

static void test_quant_hold(void)
{
    bass303_t b;
    chi_crush_t crush;
    int n = 4096, skip = 512, udry, ucrush;
    double r_dry, r_crush;
    render_note(&b, dry, n);
    memcpy(buf, dry, (size_t)n * sizeof(float));
    chi_crush_init(&crush);
    /* Yigh-class: high Edit1 + full wet — should collapse amplitude levels. */
    chi_crush_set(&crush, 108, 127);
    chi_crush_process(&crush, buf + skip, n - skip);
    udry = unique_levels(dry + skip, n - skip, 1e-5f);
    ucrush = unique_levels(buf + skip, n - skip, 1e-5f);
    CHECK(udry > 40, "dry has many distinct levels (%d)", udry);
    CHECK(ucrush < udry / 2, "full wet crush collapses levels (dry %d crush %d)", udry, ucrush);
    r_dry = rms(dry + skip, n - skip);
    r_crush = rms(buf + skip, n - skip);
    CHECK(r_dry > 1e-4, "dry has energy");
    CHECK(r_crush > 0.2 * r_dry, "crush keeps audible energy (dry %g crush %g)", r_dry, r_crush);
    /* wet=0 must be a no-op */
    memcpy(buf, dry, (size_t)n * sizeof(float));
    chi_crush_set(&crush, 108, 0);
    chi_crush_process(&crush, buf + skip, n - skip);
    CHECK(memcmp(buf + skip, dry + skip, (size_t)(n - skip) * sizeof(float)) == 0, "wet=0 leaves PCM unchanged");
}

int main(void)
{
    test_supported_flags();
    test_map_clears_sends();
    test_quant_hold();
    if (fails) {
        printf("%d FAIL(s)\n", fails);
        return 1;
    }
    printf("PASS chicago_crush\n");
    return 0;
}
