/* SPDX-License-Identifier: GPL-3.0-only */
/* Production render: changing Decay after ADSR sustain must not reopen the filter. */
#include <math.h>
#include <stdio.h>
#include "bass303.h"

static int fails;
#define CHECK(c, msg) do { if (!(c)) { puts("FAIL " msg); fails++; } } while (0)

static void advance(bass303_t *b, int n)
{
    float out[128];
    while (n) {
        int m = n > 128 ? 128 : n;
        bass303_render(b, out, m);
        n -= m;
    }
}

static void setup(bass303_t *b, int lite, int adsr, int accent)
{
    bass303_init(b);
    bass303_set_lite(b, lite);
    bass303_set(b, BASS303_FENV, adsr);
    bass303_set(b, BASS303_FATTACK, 0);
    bass303_set(b, BASS303_DECAY, 0);
    bass303_set(b, BASS303_FSUSTAIN, 64);
    bass303_set(b, BASS303_CUTOFF, 0);
    bass303_set(b, BASS303_ENVMOD, 127);
    bass303_set(b, BASS303_VOLUME, 127);
    bass303_note_on(b, 45, accent, 0);
    advance(b, 44100);
}

static void sustain_case(int lite, int adsr, int accent)
{
    bass303_t changed, control;
    float a[128], b[128];
    int i, n, differing = 0;
    double err = 0.0;
    setup(&changed, lite, adsr, accent);
    control = changed;
    if (adsr)
        CHECK(changed.fenv_stage == 3 && changed.fenv_y == changed.fsustain,
              "fixture reached sustain");
    bass303_set(&changed, BASS303_DECAY, 127);
    /* No internal-state oracle: the control simply keeps the old Decay pot.
     * Decay affects the next trigger, not an already completed decay. */
    for (n = 0; n < 88200;) {
        int m = 88200 - n > 128 ? 128 : 88200 - n;
        bass303_render(&changed, a, m);
        bass303_render(&control, b, m);
        for (i = 0; i < m; i++) {
            double d = (double)a[i] - (double)b[i];
            differing += a[i] != b[i];
            err += d * d;
        }
        n += m;
    }
    printf("lite=%d adsr=%d accent=%d differing=%d/88200 diff_rms=%.9g envelope=%.9g\n",
           lite, adsr, accent, differing, sqrt(err / 88200.0), (double)changed.fenv_y);
    CHECK(differing == 0, "live Decay at sustain preserves every output sample");
    if (!adsr)
        return;
    CHECK(changed.fenv_stage == 3 && changed.fenv_y == changed.fsustain,
          "sustain remains selected level");
    bass303_set(&changed, BASS303_FSUSTAIN, 32);
    advance(&changed, 128);
    CHECK(changed.fenv_y == changed.fsustain, "live Sustain follows selected level");
    bass303_set(&changed, BASS303_FRELEASE, 64);
    bass303_note_off(&changed);
    advance(&changed, 128);
    CHECK(changed.fenv_stage == 0 && changed.fenv_y < changed.fsustain,
          "release leaves sustain");
    advance(&changed, 88200);
    CHECK(changed.fenv_y == 0.0f && changed.idle, "release settles to silence");
    bass303_note_on(&changed, 45, 0, 0);
    advance(&changed, 22050);
    CHECK(changed.fenv_stage == 2 && changed.fenv_y > changed.fsustain,
          "new trigger uses the longer Decay");
    bass303_note_on(&changed, 57, 1, 1);
    CHECK(changed.fenv_stage == 2, "slide does not retrigger filter ADSR");
}

int main(void)
{
    int lite, adsr, accent;
    for (lite = 0; lite < 2; lite++)
        for (adsr = 0; adsr < 2; adsr++)
            for (accent = 0; accent < 2; accent++)
                sustain_case(lite, adsr, accent);
    return fails != 0;
}
