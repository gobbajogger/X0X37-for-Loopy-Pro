/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include "app/bass_303_adapt.h"

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); \
    return 1; \
} } while (0)

int main(void)
{
    float cut, env, porta, g;
    int8_t z = 0;

    g = bass_303_accent_gain(0);
    CHECK(g == 1.0f);
    CHECK(bass_303_accent_gain(1) > 3.0f);
    CHECK(bass_303_accent_gain(1) < 4.0f);
    {
        const float A = bass_303_accent_gain(1);
        float out = 0.5f, mk;
        mk = bass_303_accent_output(0, &out);
        CHECK(out == 0.5f && mk == 1.0f);            /* unaccented: untouched */
        mk = bass_303_accent_output(1, &out);        /* full-level preset: no room */
        CHECK(out == 1.0f);                          /* the Chicago gain stage takes 0..1 */
        CHECK(mk == 0.5f * A);                       /* the rest after the voice */
        CHECK(out * mk > 1.5f);                      /* still the whole accent */
        out = 1.0f;
        mk = bass_303_accent_output(1, &out);
        CHECK(out == 1.0f && mk == A);
        out = 0.2f;
        mk = bass_303_accent_output(1, &out);        /* room: same as the plain multiply */
        CHECK(out == 0.2f * A && mk == 1.0f);
        out = 0.0f;
        mk = bass_303_accent_output(1, &out);
        CHECK(out == 0.0f && mk == 1.0f);
    }
    {   /* make-up: settled 1 is a no-op; slews without a step; holds; returns */
        float l[300], r[300], g = 1.0f;
        unsigned i, n;
        for (i = 0; i < 300; ++i) l[i] = r[i] = 0.25f;
        bass_303_accent_makeup(l, r, 300, &g, 1.0f);
        for (i = 0; i < 300; ++i) CHECK(l[i] == 0.25f && r[i] == 0.25f);
        bass_303_accent_makeup(l, r, 300, &g, 3.0f);
        CHECK(g == 3.0f && l[299] == 0.75f && r[299] == 0.75f);
        for (i = 1; i < 300; ++i)
            CHECK(l[i] - l[i - 1] <= 0.25f * BASS_303_MAKEUP_SLEW * 1.001f && l[i] >= l[i - 1]);
        for (n = 0; n < 200; ++n) {                  /* 1-sample sub-blocks: same slew */
            float a = 1.0f, b = 1.0f;
            bass_303_accent_makeup(&a, &b, 1, &g, 1.0f);
            if (g == 1.0f) break;
        }
        CHECK(g == 1.0f && n < 64);
        for (i = 0; i < 300; ++i) l[i] = r[i] = 0.25f;
        bass_303_accent_makeup(l, r, 300, &g, 1.0f);
        for (i = 0; i < 300; ++i) CHECK(l[i] == 0.25f);
        bass_303_accent_makeup(l, r, 0, &g, 2.0f);   /* empty sub-block */
        CHECK(g == 1.0f);
    }

    cut = 0.25f; env = 0.10f;
    bass_303_accent_controls(0, &cut, &env);
    CHECK(cut == 0.25f && env == 0.10f);
    bass_303_accent_controls(1, &cut, &env);
    CHECK(cut > 0.25f && env > 0.10f);

    CHECK(bass_303_tune_semitones(z) == 0.0f);
    CHECK(bass_303_tune_semitones(16) > 0.0f);
    CHECK(bass_303_tune_semitones(-16) < 0.0f);

    porta = 0.0f;
    bass_303_slide_porta(0, &porta);
    CHECK(porta == 0.0f);
    bass_303_slide_porta(1, &porta);
    CHECK(porta == BASS_303_SLIDE_PORTA);

    printf("PASS bass 303 adapt: %u assertions\n", checks);
    return 0;
}
