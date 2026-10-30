/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago FX runtime: every one of the 81 factory programs loads through the
 * firmware cell and the compact FX owner, and renders finite audio. Each
 * restored wip insert (Ensemble, Short Delay, BPM Delay, Hall, Gate Reverb)
 * changes the audio against FX OFF on the same program, and Short Delay,
 * BPM Delay and Hall keep a tail after note-off. BPM Delay follows the live tempo. */
#include "original_chicago_backend.h"
#include "chicago_fx2_compact.h"
#include "app/original_sound.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

static original_chicago_backend backend;
static original_chicago_fx2_compact fx;
static original_chicago_tuning tuning;
static uint32_t core[2][256];
static int16_t converted[2][256], scratch[4][256];
static float pcm[2][256];

typedef struct { double note, tail; unsigned first_echo; float first[8192]; } render_t;

/* fx_choice < 0 keeps the factory FX. Returns energies; first[] keeps the
 * first 8192 left samples after note-off for delay timing. */
static void render(unsigned raw, int fx_choice, float bpm, render_t *out)
{
    original_chicago_backend_workspace w = {core[0], core[1], converted[0], converted[1],
        {scratch[0], scratch[1], scratch[2], scratch[3]}, 256};
    CHECK(original_chicago_tuning_bind(&tuning, 0, 0, NULL, 0x6d70));
    CHECK(original_chicago_fx2_compact_init(&fx));
    original_chicago_fx2_cold_owner owner = original_chicago_fx2_compact_owner(&fx);
    CHECK(original_chicago_backend_init_compact(&backend, raw, bpm, 0, &tuning, &owner) == 1);
    if (fx_choice >= 0) {
        float value;
        CHECK(original_sound_control_value(ORIGINAL_SOUND_CHICAGO, ORIGINAL_CHICAGO_FX_TYPE, (unsigned)fx_choice, &value));
        CHECK(original_chicago_backend_set(&backend, ORIGINAL_CHICAGO_FX_TYPE, value) == 1);
    }
    memset(out, 0, sizeof *out);
    unsigned fill = 0;
    for (unsigned b = 0; b < 140; ++b) {
        original_engine_event e[1]; unsigned ne = 0;
        if (b == 2) e[ne++] = (original_engine_event){0, ORIGINAL_EVENT_NOTE, 48, 110};
        if (b == 40) e[ne++] = (original_engine_event){0, ORIGINAL_EVENT_NOTE, 48, 0};
        CHECK(original_chicago_backend_render(&backend, &w, e, ne, pcm[0], pcm[1], 256) == 1);
        for (unsigned i = 0; i < 256; ++i) {
            CHECK(isfinite(pcm[0][i]) && isfinite(pcm[1][i]));
            CHECK(fabsf(pcm[0][i]) < 4.0f && fabsf(pcm[1][i]) < 4.0f);
            double e2 = (double)pcm[0][i] * pcm[0][i] + (double)pcm[1][i] * pcm[1][i];
            if (b < 40) out->note += e2;
            else if (b >= 48) out->tail += e2;
            if (b >= 40 && fill < 8192) out->first[fill++] = pcm[0][i];
        }
    }
    CHECK(original_chicago_backend_retire(&backend) == 1);
}

static double diff(const float *a, const float *b, unsigned n)
{
    double d = 0; for (unsigned i = 0; i < n; ++i) d += fabs((double)a[i] - b[i]); return d;
}

int main(void)
{
    static const unsigned restored_types[5] = {3, 12, 13, 15, 16};
    static const char *const restored_names[5] = {"ENSEMBLE", "SHORT DLY", "BPM DELAY", "HALL", "GATE REV"};
    static render_t a, off, slow, fast;
    unsigned per_type[32] = {0};
    CHECK(original_sound_factories(ORIGINAL_SOUND_CHICAGO) == 81);
    for (unsigned t = 0; t < 32; ++t) CHECK(original_chicago_fx2_compact_supports(t) ==
        (t == 0 || t == 3 || t == 4 || t == 5 || t == 10 || t == 12 || t == 13 || t == 15 || t == 16 ||
         t == 18 || t == 22 || t == 24));
    /* 1. All 81 factory programs: cell, routing and audio. */
    for (unsigned visible = 0; visible < 81; ++visible) {
        uint8_t cell[ORIGINAL_SOUND_BYTES];
        original_chicago_program p; original_chicago_routing r;
        int raw = original_sound_factory_source(ORIGINAL_SOUND_CHICAGO, visible);
        CHECK(raw >= 0 && original_sound_init(cell, ORIGINAL_SOUND_CHICAGO, visible) && original_sound_valid(cell));
        CHECK(original_sound_restore_chicago(&p, cell) && original_chicago_program_routing(&p, &r));
        CHECK(original_chicago_fx2_compact_supports(r.fx[1][0]));
        per_type[r.fx[1][0]]++;
        render((unsigned)raw, -1, 120.0f, &a);
        CHECK(a.note > 1e-3);
    }
    unsigned restored_programs = 0;
    for (unsigned i = 0; i < 5; ++i) restored_programs += per_type[restored_types[i]];
    CHECK(restored_programs == 26);
    printf("81/81 Chicago factory programs load and render (26 with restored inserts)\n");
    /* 2. Each restored insert changes the audio of one program; tails stay. */
    for (unsigned i = 0; i < 5; ++i) {
        unsigned choice = 7 + i;
        CHECK(!strcmp(original_sound_fx_name(choice), restored_names[i]));
        render(0, (int)choice, 120.0f, &a);
        render(0, 0, 120.0f, &off);
        double change = diff(a.first, off.first, 2048);
        printf("%-9s note %.3f off %.3f tail %.5f off-tail %.5f change %.3f\n", restored_names[i],
            a.note, off.note, a.tail, off.tail, change);
        CHECK(change > 1.0);
        /* Gate Reverb closes with the dry signal; Ensemble has no tail. */
        if (restored_types[i] != 3 && restored_types[i] != 16) CHECK(a.tail > off.tail * 1.5 + 1e-6);
    }
    /* 3. BPM Delay: the echo time follows the tempo (live tempo, #677).
     * Program 0 at 100 and 170 BPM must give different echo lags. */
    {
        unsigned lag_slow = 0, lag_fast = 0; double best;
        render(0, 9, 100.0f, &slow);
        render(0, 9, 170.0f, &fast);
        best = -1;
        for (unsigned L = 400; L < 4100; ++L) { double s = 0; for (unsigned k = 0; k + L < 8192; ++k) s += (double)slow.first[k] * slow.first[k + L]; if (s > best) { best = s; lag_slow = L; } }
        best = -1;
        for (unsigned L = 400; L < 4100; ++L) { double s = 0; for (unsigned k = 0; k + L < 8192; ++k) s += (double)fast.first[k] * fast.first[k + L]; if (s > best) { best = s; lag_fast = L; } }
        printf("BPM DELAY echo lag: %u samples at 100 BPM, %u at 170 BPM\n", lag_slow, lag_fast);
        CHECK(diff(slow.first, fast.first, 8192) > 1.0 && lag_slow != lag_fast);
    }
    puts("PASS chicago-fx-runtime: 12 FX choices, 81 factory programs, 5 restored inserts audible");
    return 0;
}
