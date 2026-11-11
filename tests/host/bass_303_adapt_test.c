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
        float out = 0.5f;
        bass_303_accent_output(0, &out);
        CHECK(out == 0.5f);
        bass_303_accent_output(1, &out);
        CHECK(out == 1.0f);                      /* the Chicago gain stage takes 0..1 */
        out = 0.2f;
        bass_303_accent_output(1, &out);
        CHECK(out > 0.6f && out < 0.8f);
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
