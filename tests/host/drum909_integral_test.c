/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../firmware/src/dsp/drum909.h"

void ref_drum909_render_st_masked(drum909_t *, float *, float *, float *, float *, int, uint16_t);
void ref_drum909_render(drum909_t *, float *, float *, float *, int);
static uint32_t rng = 1;
static uint32_t random_u32(void)
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return rng;
}
static void rate(drum909_t *d, int k)
{
    static const float p[] = {1.0f, 2.0f, 4.0f, 0.25f, 0.5f, 0.7937005f, 1.3348398f, 3.1748021f};
    for (int j = 0; j < 4; ++j) {
        d9_smp_t *s = &d->smp[j];
        s->pitch = p[(k + j) % 8];
        s->inc = (uint32_t)s->pitch;
        s->incf = (uint32_t)((s->pitch - (float)s->inc) * 4294967296.0f);
    }
}
static void check(void)
{
    unsigned calls = 0;
    for (int seed = 1; seed <= 32; ++seed) {
        drum909_t a, b;
        memset(&a, 0, sizeof a);
        drum909_init(&a);
        rng = (uint32_t)seed;
        for (int v = 0; v < DR_NUM; ++v) {
            for (int p = 0; p < drum909_nparams(v); ++p) {
                const x0x_param_t *spec = drum909_param(v, p);
                int value = (int)(random_u32() % (spec->max + 1u));
                if (!strcmp(spec->name, "Drive")) value = seed % 4 ? 80 : 0;
                if (!strcmp(spec->name, "Dist")) value = seed % 7;
                if (!strcmp(spec->name, "Rev") || !strcmp(spec->name, "Dly"))
                    value = seed % 3 ? 77 : 0;
                drum909_set(&a, v, p, value);
            }
            drum909_trigger(&a, v, 0.8f);
        }
        rate(&a, seed);
        b = a; /* Use the same sample pointers, so full-state memcmp is valid. */
        for (int step = 0; step < 1300; ++step) {
            float x[4][256], y[4][256];
            const int n = step % 257;
            for (int j = 0; j < 4; ++j)
                for (int i = 0; i < 256; ++i)
                    x[j][i] = (random_u32() & 1) ? -0.0f : (float)(int)(random_u32() % 97) * 0.001f;
            memcpy(y, x, sizeof x);
            if (step % 53 == 0) {
                const int v = (step / 53 + seed) % DR_NUM;
                drum909_trigger(&a, v, 0.6f);
                drum909_trigger(&b, v, 0.6f);
            }
            if (step % 71 == 0) { rate(&a, step / 71); rate(&b, step / 71); }
            if (step == 699) { drum909_all_off(&a); drum909_all_off(&b); }
            if (step == 700) {
                for (int v = 0; v < DR_NUM; ++v) {
                    drum909_trigger(&a, v, 1.0f); drum909_trigger(&b, v, 1.0f);
                }
            }
            /* Exercise decode lookahead and exact final-sample behaviour. */
            if (step == 910) {
                for (int j = 0; j < 4; ++j) {
                    drum909_trigger(&a, DR_CH + j, 1.0f);
                    drum909_trigger(&b, DR_CH + j, 1.0f);
                    a.smp[j].len = b.smp[j].len = (uint32_t)(1 + j);
                }
                rate(&a, 0); rate(&b, 0);
            }
            if (step % 2) {
                const uint16_t mask = (uint16_t)(random_u32() & ((1u << DR_NUM) - 1));
                drum909_render_st_masked(&a, x[0], (step % 3) ? x[1] : NULL,
                    (step % 5) ? x[2] : NULL, (step % 7) ? x[3] : NULL, n, mask);
                ref_drum909_render_st_masked(&b, y[0], (step % 3) ? y[1] : NULL,
                    (step % 5) ? y[2] : NULL, (step % 7) ? y[3] : NULL, n, mask);
            } else {
                drum909_render(&a, x[0], x[2], x[3], n);
                ref_drum909_render(&b, y[0], y[2], y[3], n);
            }
            if (memcmp(&a, &b, sizeof a) || memcmp(x, y, sizeof x)) {
                fprintf(stderr, "FAIL seed=%d step=%d n=%d state=%d pcm=%d\n", seed, step, n,
                    memcmp(&a, &b, sizeof a), memcmp(x, y, sizeof x));
                exit(1);
            }
            ++calls;
        }
    }
    printf("PASS %u calls: complete drum909_t (%zu bytes) and four PCM buses bit-identical\n", calls, sizeof(drum909_t));
}
static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * 1e9 + (double)t.tv_nsec;
}
static double bench(int reference, int dense, int fractional)
{
    drum909_t d;
    float bus[4][32];
    memset(&d, 0, sizeof d); drum909_init(&d);
    for (int j = 0; j < 4; ++j) {
        d.smp[j].inc = 1;
        d.smp[j].incf = fractional ? 0x80000000u : 0;
    }
    const double start = now();
    for (int k = 0; k < 6000; ++k) {
        if (k % 32 == 0)
            for (int v = dense ? 0 : DR_CH; v < DR_NUM; ++v)
                drum909_trigger(&d, v, 0.8f);
        memset(bus, 0, sizeof bus);
        if (reference) ref_drum909_render_st_masked(&d, bus[0], bus[1], bus[2], bus[3], 32, 0x7ff);
        else drum909_render_st_masked(&d, bus[0], bus[1], bus[2], bus[3], 32, 0x7ff);
    }
    return (now() - start) / 6000.0;
}
static int cmp(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
int main(void)
{
    check();
    for (int dense = 0; dense < 2; ++dense)
        for (int fractional = 0; fractional < 2; ++fractional) {
            double a[9], b[9];
            for (int k = 0; k < 9; ++k) {
                if (k % 2) { b[k] = bench(0, dense, fractional); a[k] = bench(1, dense, fractional); }
                else { a[k] = bench(1, dense, fractional); b[k] = bench(0, dense, fractional); }
            }
            qsort(a, 9, sizeof *a, cmp); qsort(b, 9, sizeof *b, cmp);
            printf("MEASURED host %s %s median ns/block: base %.0f candidate %.0f (%+.2f%%)\n",
                dense ? "dense" : "samplers", fractional ? "fractional" : "integral", a[4], b[4],
                100.0 * (b[4] / a[4] - 1.0));
        }
    return 0;
}
