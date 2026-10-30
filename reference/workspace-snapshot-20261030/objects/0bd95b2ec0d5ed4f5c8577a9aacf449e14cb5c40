/* SPDX-License-Identifier: GPL-3.0-only */
/* Host proof: Chicago GateRev insert adds gated-reverb colour vs dry native303. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "bass303.h"
#include "chicago_gaterev.h"
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

static void test_supported_flags(void)
{
    CHECK(chi_fx_supported(CHI_FX_GATE_REV), "GateRev supported");
    CHECK(chi_fx_is_insert(CHI_FX_GATE_REV), "GateRev is insert");
    CHECK(chi_fx_is_gaterev_insert(CHI_FX_GATE_REV), "GateRev gaterev-insert helper");
    CHECK(!chi_fx_is_not_yet(CHI_FX_GATE_REV), "GateRev not NOT_YET");
    CHECK(strstr(CHI_FX_TYPE_N[CHI_FX_GATE_REV], "—") == 0, "GateRev label has no em-dash");
}

static void test_map_clears_sends(void)
{
    chicago_fx_out_t o;
    chicago_fx_map(CHI_FX_GATE_REV, 28, 31, 0, &o);
    CHECK(o.touch == CHI_FX_W_CLEAR_SENDS, "GateRev insert clears sends");
    CHECK(!(o.touch & CHI_FX_W_MX_REV), "GateRev does not set Mix reverb");
    CHECK(!(o.touch & CHI_FX_W_RV_DEC), "GateRev does not set RvDec");
    CHECK(!(o.touch & CHI_FX_W_RV_LVL), "GateRev does not set RvLvl");
}

static void test_gaterev_changes_pcm(void)
{
    bass303_t b;
    chi_gaterev_t grev;
    int n = 4096, skip = 512;
    double delta, delta_hi;
    render_note(&b, dry, n);

    memcpy(buf, dry, (size_t)n * sizeof(float));
    chi_gaterev_init(&grev);
    chi_gaterev_set(&grev, 28, 31, BASS303_SR); /* Summer Bass-ish */
    chi_gaterev_process(&grev, buf + skip, n - skip);
    delta = rms_delta(dry + skip, buf + skip, n - skip);
    CHECK(delta > 1e-4, "GateRev changes waveform (rms delta %g)", delta);

    memcpy(buf, dry, (size_t)n * sizeof(float));
    chi_gaterev_init(&grev);
    chi_gaterev_set(&grev, 127, 127, BASS303_SR);
    chi_gaterev_process(&grev, buf + skip, n - skip);
    delta_hi = rms_delta(dry + skip, buf + skip, n - skip);
    CHECK(delta_hi > delta * 1.02, "higher wet/decay moves PCM more (lo %g hi %g)", delta, delta_hi);

    memcpy(buf, dry, (size_t)n * sizeof(float));
    chi_gaterev_init(&grev);
    chi_gaterev_process(&grev, buf + skip, n - skip);
    CHECK(memcmp(buf + skip, dry + skip, (size_t)(n - skip) * sizeof(float)) == 0,
          "unset GateRev leaves PCM unchanged");
}

int main(void)
{
    test_supported_flags();
    test_map_clears_sends();
    test_gaterev_changes_pcm();
    if (fails) {
        printf("%d FAIL(s)\n", fails);
        return 1;
    }
    printf("PASS chicago_gaterev\n");
    return 0;
}
