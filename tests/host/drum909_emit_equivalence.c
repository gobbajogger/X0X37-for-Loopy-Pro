/* SPDX-License-Identifier: GPL-3.0-only */
/* Compare the isolated emit/rim/clap candidates with a pinned source baseline. */
#include "drum909.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ref_drum909_init(drum909_t *);
void ref_drum909_set(drum909_t *, int, int, int);
void ref_drum909_trigger(drum909_t *, int, float);
void ref_drum909_pan_settle(drum909_t *);
void ref_drum909_render(drum909_t *, float *, float *, float *, int);
void ref_drum909_render_st_masked(drum909_t *, float *, float *, float *, float *, int, uint16_t);

static unsigned rng;
static unsigned next(void) { rng = rng * 1664525u + 1013904223u; return rng; }
static void check(const void *a, const void *b, size_t n, const char *what, int seed, int dist, int block)
{
    if (memcmp(a, b, n)) {
        size_t i = 0;
        while (((const unsigned char *)a)[i] == ((const unsigned char *)b)[i]) ++i;
        fprintf(stderr, "%s mismatch seed=%d dist=%d block=%d byte=%zu\n", what, seed, dist, block, i);
        exit(1);
    }
}
int main(void)
{
    unsigned blocks = 0;
    /* Exercise ringing through signed zero/subnormal histories, impulse
     * boundaries, partial final blocks and the eventual silent state. */
    for (int phase = 0; phase < 12; ++phase) {
        drum909_t a, b;
        memset(&a, 0, sizeof a);
        drum909_init(&a);
        drum909_trigger(&a, DR_RS, 1.0f);
        a.rim.impulse = phase % 3;
        a.rim.bp1.y1 = phase & 1 ? -1.0e-40f : 1.0e-40f;
        a.rim.bp1.y2 = phase & 2 ? -0.0f : 0.0f;
        a.rim.bp2.y1 = -a.rim.bp1.y1;
        a.rim.bp2.y2 = -a.rim.bp1.y2;
        a.pan_lc[DR_RS] = a.pan_l[DR_RS] = phase & 4 ? -0.0f : 0.0f;
        a.pan_rc[DR_RS] = a.pan_r[DR_RS] = 0.0f;
        memcpy(&b, &a, sizeof b);
        static const int sizes[] = { 0, 1, 2, 3, 4, 255, 256, 17 };
        for (int block = 0; block < 120; ++block) {
            float x[4][256] = {{0}}, y[4][256] = {{0}};
            int n = sizes[block % 8];
            drum909_render_st_masked(&a, x[0], x[1], x[2], x[3], n, 0x7ff);
            ref_drum909_render_st_masked(&b, y[0], y[1], y[2], y[3], n, 0x7ff);
            check(x, y, sizeof x, "ring PCM", phase, -1, block);
            check(&a, &b, sizeof a, "ring state", phase, -1, block);
            ++blocks;
        }
    }
    for (int seed = 1; seed <= 6; ++seed) for (int dist = 0; dist < 7; ++dist) {
        drum909_t a, b;
        memset(&a, 0, sizeof a); memset(&b, 0, sizeof b);
        drum909_init(&a); ref_drum909_init(&b);
        /* Each TU has its own static sample arrays. Check their bytes, then use
         * the same immutable arrays so full-state memcmp includes all pointers. */
        for (int v = 0; v < 4; ++v) {
            check(&a.smp[v].len, &b.smp[v].len, sizeof a.smp[v].len, "sample length", seed, dist, -1);
            check(a.smp[v].adpcm, b.smp[v].adpcm, (a.smp[v].len + 1u) / 2u, "sample bytes", seed, dist, -1);
            b.smp[v].adpcm = a.smp[v].adpcm;
        }
        check(&a, &b, sizeof a, "init", seed, dist, -1);
        rng = (unsigned)seed;
        for (int block = 0; block < 520; ++block) {
            if (block % 31 == 0) {
                for (int v = 0; v <= DR_KIT; ++v) for (int p = 0; p < drum909_nparams(v); ++p) {
                    const x0x_param_t *spec = drum909_param(v, p);
                    int val = (int)(next() % 128u);
                    if (!strcmp(spec->name, "Dist")) val = dist;
                    if (!strcmp(spec->name, "Rev") || !strcmp(spec->name, "Dly"))
                        val = (block / 31) % 2 ? val : 0;
                    drum909_set(&a, v, p, val); ref_drum909_set(&b, v, p, val);
                }
            }
            if (block % 29 == 0) { drum909_pan_settle(&a); ref_drum909_pan_settle(&b); }
            /* The pinned base has no panic API; compare its real reset lifecycle. */
            if (block % 97 == 96) {
                drum909_init(&a); ref_drum909_init(&b);
                /* Reset restores each TU's separately allocated immutable arrays. */
                for (int v = 0; v < 4; ++v) b.smp[v].adpcm = a.smp[v].adpcm;
            }
            if (block % 19 == 0) for (int v = 0; v < DR_NUM; ++v) {
                const float vel = (float)(next() % 128u) / 127.0f;
                drum909_trigger(&a, v, vel); ref_drum909_trigger(&b, v, vel);
            }
            /* Long uninterrupted rim/clap decay, with retriggers and silence. */
            if (block % 131 == 1) {
                drum909_trigger(&a, DR_RS, 1.0f); ref_drum909_trigger(&b, DR_RS, 1.0f);
                drum909_trigger(&a, DR_CP, 1.0f); ref_drum909_trigger(&b, DR_CP, 1.0f);
            }
            float x[4][256], y[4][256];
            for (int bus = 0; bus < 4; ++bus) for (int i = 0; i < 256; ++i)
                x[bus][i] = (i % 3 == 0) ? -0.0f : (float)(int)(next() % 255u) * 0.001f;
            memcpy(y, x, sizeof x);
            const int n = block % 257;
            float *ar = block % 5 ? x[2] : NULL, *br = block % 5 ? y[2] : NULL;
            float *ad = block % 7 ? x[3] : NULL, *bd = block % 7 ? y[3] : NULL;
            if (block % 3 == 0) {
                drum909_render(&a, x[0], ar, ad, n);
                ref_drum909_render(&b, y[0], br, bd, n);
            } else {
                const uint16_t mask = block % 11 ? 0x7ffu : (uint16_t)(next() & 0x7ffu);
                drum909_render_st_masked(&a, x[0], x[1], ar, ad, n, mask);
                ref_drum909_render_st_masked(&b, y[0], y[1], br, bd, n, mask);
            }
            check(x, y, sizeof x, "PCM", seed, dist, block);
            check(&a, &b, sizeof a, "state", seed, dist, block);
            ++blocks;
        }
    }
    printf("PASS: %u blocks; full state (%zu bytes) and four PCM buses identical\n", blocks, sizeof(drum909_t));
    return 0;
}
