/* SPDX-License-Identifier: GPL-3.0-only */
/* Host proof: Chicago Ring Mod insert creates carrier sidebands vs dry native303. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "bass303.h"
#include "chicago_fx.h"
#include "chicago_ring.h"

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

static double energy_band(const float *x, int n, double f0, double f1, double sr)
{
    /* Coarse Goertzel-ish: sum |x| * cos/sin at band centre (enough to detect sideband lift). */
    double c = 0, s = 0, f = 0.5 * (f0 + f1), w = 2.0 * 3.14159265358979 * f / sr;
    int i;
    for (i = 0; i < n; i++) {
        double t = (double)i * w;
        c += (double)x[i] * cos(t);
        s += (double)x[i] * sin(t);
    }
    return c * c + s * s;
}

static void test_supported_flags(void)
{
    CHECK(chi_fx_supported(CHI_FX_RING), "Ring supported");
    CHECK(chi_fx_is_insert(CHI_FX_RING), "Ring is insert");
    CHECK(!chi_fx_is_not_yet(CHI_FX_RING), "Ring not NOT_YET");
    CHECK(strstr(CHI_FX_TYPE_N[CHI_FX_RING], "—") == 0, "Ring label has no em-dash");
}

static void test_map_clears_sends(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_RING, 80, 127, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "Ring map clears sends");
    CHECK(!(o.touch & CHI_FX_W_DIST), "Ring does not steal Crush");
}

static void test_sidebands(void)
{
    bass303_t b;
    chi_ring_t ring;
    double e_dry, e_ring, fund_hz = 130.81; /* MIDI 48 ≈ C3 */
    double sr = (double)BASS303_SR;
    int n = 4096, skip = 512;
    render_note(&b, dry, n);
    memcpy(buf, dry, (size_t)n * sizeof(float));
    chi_ring_init(&ring);
    /* Edit1 mid-high (~1–2 kHz carrier), full wet — should move energy off the fund. */
    chi_ring_set(&ring, 96, 127, (float)sr);
    chi_ring_process(&ring, buf + skip, n - skip);
    e_dry = energy_band(dry + skip, n - skip, fund_hz - 5.0, fund_hz + 5.0, sr);
    e_ring = energy_band(buf + skip, n - skip, fund_hz - 5.0, fund_hz + 5.0, sr);
    CHECK(e_dry > 1e-6, "dry has fund energy");
    /* Full ring nulls the carrier of the modulator path; fund energy must drop. */
    CHECK(e_ring < 0.55 * e_dry, "full wet ring reduces fund energy (dry %g ring %g)", e_dry, e_ring);
    {
        double car = (double)chi_ring_hz_from_edit1(96);
        double e_sb = energy_band(buf + skip, n - skip, fund_hz + car - 30.0, fund_hz + car + 30.0, sr);
        double e_sb_dry = energy_band(dry + skip, n - skip, fund_hz + car - 30.0, fund_hz + car + 30.0, sr);
        CHECK(e_sb > 3.0 * (e_sb_dry + 1e-12), "ring lifts upper sideband vs dry");
    }
}

int main(void)
{
    test_supported_flags();
    test_map_clears_sends();
    test_sidebands();
    if (fails) {
        printf("%d FAIL(s)\n", fails);
        return 1;
    }
    printf("PASS chicago_ring\n");
    return 0;
}
