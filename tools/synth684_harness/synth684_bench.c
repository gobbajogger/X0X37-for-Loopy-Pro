/* SPDX-License-Identifier: GPL-3.0-only */
/* PR #684 host harness: golden renders and block timing for Chicago and Miami.
 * Each scenario renders a fixed note and parameter script to a raw file of
 * interleaved little-endian binary32 stereo samples at 44100 Hz.
 * When the build uses stub inputs, every output is marked NOT VALID. */
#include "app/original_runtime.h"
#include "chicago_fx2_compact.h"
#include "chicago_tuning.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef X0X_SYNTH684_ICOUNT
#include <valgrind/callgrind.h>
#define REPEATS 1
#else
#define CALLGRIND_TOGGLE_COLLECT
#define CALLGRIND_DUMP_STATS_AT(x) ((void)(x))
#endif
#ifdef X0X_SYNTH684_STUB_INPUTS
#define VALIDITY "STUB-NOT-VALID"
#else
#define VALIDITY "REAL"
#endif
#define BLOCK 256u
#define BPM 120.0f

static original_runtime_part part;
static original_runtime_workspace work;
static original_chicago_tuning tuning;
static original_chicago_fx2_compact fx;
static original_chicago_fx2_cold_owner fx_owner;

typedef struct { unsigned block, kind; int key, vel; int param; float value; int damper; } step;
#define NOTE(b,k,v) {b,0,k,v,-1,0,-1}
#define PARAM(b,p,x) {b,0,-1,0,p,x,-1}
#define DAMP(b,d) {b,0,-1,0,-1,0,d}
#define END {0xffffffffu,0,-1,0,-1,0,-1}

/* Chicago: chord, Pitch Bend sweep (param 20), damper, FX type (param 17). */
static const step chicago_script[] = {
    NOTE(0,48,100), NOTE(0,55,90), NOTE(0,60,80),
    PARAM(8,20,0.75f), PARAM(16,20,1.0f), PARAM(24,20,0.5f), PARAM(32,20,0.0f), PARAM(40,20,0.5f),
    DAMP(48,127), NOTE(56,48,0), NOTE(56,55,0), NOTE(56,60,0), NOTE(64,67,100), NOTE(72,67,0),
    DAMP(88,0), PARAM(96,2,0.2f), NOTE(100,36,110), PARAM(110,2,0.9f), NOTE(130,36,0), END };
/* Miami: bass line, wobble depth (param 12) at fixed tempo, Pitch Bend (param 16), damper. */
static const step miami_script[] = {
    NOTE(0,36,100), PARAM(4,12,1.0f), NOTE(32,36,0), NOTE(32,43,100), PARAM(40,16,1.0f),
    PARAM(48,16,0.0f), PARAM(56,16,0.5f), DAMP(64,127), NOTE(70,43,0), NOTE(80,48,100),
    NOTE(90,48,0), DAMP(110,0), PARAM(112,12,0.0f), END };

static int apply(const step *s, unsigned kind)
{
    if (s->key >= 0) return original_runtime_key(&part, ORIGINAL_INPUT_SEQUENCE, (unsigned)s->key, (unsigned)s->vel);
    if (s->param >= 0) return original_runtime_param(&part, (unsigned)s->param, s->value);
    if (s->damper >= 0)
        /* The damper is the performance control after Pitch Bend, as the firmware uses it. */
        return original_runtime_param(&part, kind == ORIGINAL_SOUND_MIAMI ? ORIGINAL_MIAMI_PITCH_BEND + 1u :
                                      ORIGINAL_CHICAGO_PITCH_BEND + 1u, (float)s->damper / 127.0f);
    return 1;
}
static double now_ns(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e9 + t.tv_nsec; }
static int cmp_d(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }

static int run_once(FILE *f, unsigned kind, unsigned factory, unsigned fxchoice, unsigned blocks,
                    double *t, uint64_t *hash_out)
{
    uint8_t cell[ORIGINAL_SOUND_BYTES];
    if (!original_sound_init(cell, kind, factory)) { fprintf(stderr, "sound_init fail %u/%u\n", kind, factory); return 1; }
    if (kind == ORIGINAL_SOUND_CHICAGO && fxchoice && !original_sound_set(cell, ORIGINAL_CHICAGO_FX_TYPE, fxchoice)) {
        fprintf(stderr, "fx choice %u rejected\n", fxchoice); return 1; }
    memset(&part, 0, sizeof part);
    if (original_runtime_init(&part, cell, BPM, &tuning, &fx_owner) != ORIGINAL_RUNTIME_OK) {
        fprintf(stderr, "runtime_init fail kind %u factory %u status %d\n", kind, factory, part.status); return 1; }
    const step *s = kind ? miami_script : chicago_script;
    static float l[BLOCK], r[BLOCK], inter[2 * BLOCK];
    uint64_t hash = UINT64_C(1469598103934665603);
    for (unsigned b = 0; b < blocks; ++b) {
        /* As in engine.c: prepare the block first, then queue its events, then render. */
        (void)original_runtime_prepare(&part, BPM);
        for (; s->block == b; ++s)
            if (apply(s, kind) != 1) fprintf(stderr, "%s p%u fx%u: event at block %u returned not OK\n",
                                            kind ? "miami" : "chicago", factory, fxchoice, b);
        double t0 = now_ns();
        CALLGRIND_TOGGLE_COLLECT;
        int st = original_runtime_render(&part, &work, l, r, BLOCK);
        CALLGRIND_TOGGLE_COLLECT;
        t[b] = now_ns() - t0;
        if (st != ORIGINAL_RUNTIME_OK) {
            fprintf(stderr, "%s p%u fx%u: render status %d at block %u\n", kind ? "miami" : "chicago", factory, fxchoice, st, b);
            return 1; }
        for (unsigned i = 0; i < BLOCK; ++i) { inter[2*i] = l[i]; inter[2*i+1] = r[i]; }
        if (f) fwrite(inter, sizeof(float), 2 * BLOCK, f);
        const uint8_t *p = (const uint8_t *)inter;
        for (unsigned i = 0; i < sizeof inter; ++i) { hash ^= p[i]; hash *= UINT64_C(1099511628211); }
    }
    (void)original_runtime_retire(&part);
    *hash_out = hash;
    return 0;
}

#ifndef REPEATS
#define REPEATS 15
#endif
static int scenario(const char *out, unsigned kind, unsigned factory, unsigned fxchoice, unsigned blocks, FILE *csv)
{
    char name[512];
    snprintf(name, sizeof name, "%s/%s-p%02u-fx%u.f32", out, kind ? "miami" : "chicago", factory, fxchoice);
    FILE *f = fopen(name, "wb"); if (!f) return 1;
    double *t = malloc(sizeof(double) * blocks), mean[REPEATS], all_med[REPEATS], all_p99[REPEATS];
    uint64_t hash0 = 0, hash;
    int rc = 0;
    for (unsigned k = 0; k < REPEATS && !rc; ++k) {
        rc = run_once(k ? NULL : f, kind, factory, fxchoice, blocks, t, &hash);
        if (rc) break;
        if (!k) hash0 = hash;
        else if (hash != hash0) { fprintf(stderr, "non-deterministic render\n"); rc = 1; }
        double sum = 0; for (unsigned b = 0; b < blocks; ++b) sum += t[b];
        mean[k] = sum / blocks;
        qsort(t, blocks, sizeof(double), cmp_d);
        all_med[k] = t[blocks / 2]; all_p99[k] = t[blocks * 99 / 100];
    }
    fclose(f);
    CALLGRIND_DUMP_STATS_AT(name);
    if (!rc) {
        qsort(mean, REPEATS, sizeof(double), cmp_d); qsort(all_med, REPEATS, sizeof(double), cmp_d);
        qsort(all_p99, REPEATS, sizeof(double), cmp_d);
        fprintf(csv, "%s,%s,%u,%u,%u,%.0f,%.0f,%.0f,%.0f,%016llx\n", VALIDITY, kind ? "miami" : "chicago", factory, fxchoice,
                blocks, mean[0], mean[REPEATS/2], all_med[REPEATS/2], all_p99[REPEATS/2], (unsigned long long)hash0);
    }
    free(t);
    return rc;
}

#ifdef X0X_SYNTH684_ICOUNT
/* Idle counts include the public float-output/runtime bridge. Preparation,
 * cold initialization and warmup are excluded, matching the active workload's
 * counted call. No notes or parameter events are queued. */
static int miami_idle_scenario(const char *out, unsigned factory, unsigned blocks, FILE *csv)
{
    const float bpm = 217.5f;
    const unsigned warmup = 64;
    uint8_t cell[ORIGINAL_SOUND_BYTES];
    static float l[BLOCK], r[BLOCK], inter[2 * BLOCK];
    char name[512];
    CALLGRIND_STOP_INSTRUMENTATION;
    if (!original_sound_init(cell, ORIGINAL_SOUND_MIAMI, factory)) return 1;
    memset(&part, 0, sizeof part);
    if (original_runtime_init(&part, cell, bpm, &tuning, &fx_owner) != ORIGINAL_RUNTIME_OK) return 1;
    for (unsigned b = 0; b < warmup; ++b) {
        if (original_runtime_prepare(&part, bpm) != ORIGINAL_RUNTIME_OK ||
            original_runtime_render(&part, &work, l, r, BLOCK) != ORIGINAL_RUNTIME_OK) return 1;
    }
    snprintf(name, sizeof name, "%s/miami-idle-p%02u-fx0.f32", out, factory);
    FILE *f = fopen(name, "wb"); if (!f) return 1;
    CALLGRIND_START_INSTRUMENTATION;
    CALLGRIND_ZERO_STATS;
    uint64_t hash = UINT64_C(1469598103934665603);
    for (unsigned b = 0; b < blocks; ++b) {
        if (original_runtime_prepare(&part, bpm) != ORIGINAL_RUNTIME_OK) { fclose(f); return 1; }
        CALLGRIND_TOGGLE_COLLECT;
        int st = original_runtime_render(&part, &work, l, r, BLOCK);
        CALLGRIND_TOGGLE_COLLECT;
        if (st != ORIGINAL_RUNTIME_OK) { fclose(f); return 1; }
        for (unsigned i = 0; i < BLOCK; ++i) { inter[2*i] = l[i]; inter[2*i+1] = r[i]; }
        const uint8_t *p = (const uint8_t *)inter;
        for (unsigned i = 0; i < sizeof inter; ++i) {
            /* Require exact positive-zero PCM, independently of any golden. */
            if (p[i]) { fprintf(stderr, "nonzero Miami idle PCM p%u block %u\n", factory, b); fclose(f); return 1; }
            hash ^= p[i]; hash *= UINT64_C(1099511628211);
        }
        if (fwrite(inter, sizeof(float), 2 * BLOCK, f) != 2 * BLOCK) { fclose(f); return 1; }
    }
    if (fclose(f)) return 1;
    CALLGRIND_DUMP_STATS_AT(name);
    fprintf(csv, "%s,miami-idle,%u,0,%u,0,0,0,0,%016llx\n", VALIDITY, factory, blocks,
            (unsigned long long)hash);
    return original_runtime_retire(&part) == ORIGINAL_RUNTIME_OK ? 0 : 1;
}
#endif

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s OUTDIR [blocks]\n", argv[0]); return 2; }
    unsigned blocks = argc > 2 ? (unsigned)atoi(argv[2]) : 172u; /* about 1 s */
    if (!original_chicago_tuning_bind(&tuning, 0, 0, NULL, 0x6d70) || !original_chicago_fx2_compact_init(&fx)) return 1;
    fx_owner = original_chicago_fx2_compact_owner(&fx);
    char path[512]; snprintf(path, sizeof path, "%s/results.csv", argv[1]);
    FILE *csv = fopen(path, "w"); if (!csv) return 1;
    fprintf(csv, "validity,engine,program,fx,blocks,min_mean_ns_per_block,median_mean_ns_per_block,median_ns,p99_ns,fnv1a64\n");
    static const unsigned chicago_programs[] = {0, 7, 19, 33, 54}, miami_programs[] = {0, 9, 18, 29};
    int rc = 0;
#ifdef X0X_SYNTH684_ICOUNT
    int idle_only = argc > 3 && !strcmp(argv[3], "--miami-idle-only");
    if (idle_only) goto miami_idle_counts;
#endif
    for (unsigned i = 0; i < 5; ++i) rc |= scenario(argv[1], ORIGINAL_SOUND_CHICAGO, chicago_programs[i], 0, blocks, csv);
    for (unsigned c = 1; c < ORIGINAL_CHICAGO_FX_CHOICES; ++c) rc |= scenario(argv[1], ORIGINAL_SOUND_CHICAGO, 0, c, blocks, csv);
    for (unsigned i = 0; i < 4; ++i) rc |= scenario(argv[1], ORIGINAL_SOUND_MIAMI, miami_programs[i], 0, blocks, csv);
#ifdef X0X_SYNTH684_ICOUNT
miami_idle_counts:
    if (idle_only || (argc > 3 && !strcmp(argv[3], "--miami-idle"))) {
        for (unsigned i = 0; i < ORIGINAL_MIAMI_BACKEND_FACTORIES; ++i)
            rc |= miami_idle_scenario(argv[1], i, blocks, csv);
        fprintf(csv, "# idle_warmup_blocks=64 idle_bpm=217.5 idle_counted_call=original_runtime_render\n");
    }
#endif
    fprintf(csv, "# validity=%s block=%u bpm=%.1f sample_rate=44100 format=f32le-stereo-interleaved\n", VALIDITY, BLOCK, BPM);
    fclose(csv);
    printf("%s: results in %s\n", VALIDITY, path);
    printf("host sizeof: chicago_backend %zu, miami_backend %zu, runtime_part %zu, workspace %zu, fx2_compact %zu\n",
           sizeof(original_chicago_backend), sizeof(original_miami_backend), sizeof(original_runtime_part),
           sizeof(original_runtime_workspace), sizeof(original_chicago_fx2_compact));
    return rc;
}
