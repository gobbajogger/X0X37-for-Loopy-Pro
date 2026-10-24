/* SPDX-License-Identifier: GPL-3.0-only */
/* BREAK render callback-tail host proxy. Actual linked production breaks.c.
 * Observed host p99 and worst block durations are NOT device ISR/XRUN proof.
 * Synthetic ADPCM only. No firmware/controller/SDK/private samples included. */
#define _POSIX_C_SOURCE 200809L
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "firmware/src/dsp/breaks.h"

#define BLOCK 64u
#define MIN_BLOCKS 64u
#define MAX_BLOCKS 4096u
static uint32_t rng(uint32_t *p)
{
    uint32_t x = *p;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *p = x;
}
static uint64_t ns(void)
{
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t)) { perror("clock_gettime"); exit(3); }
    return (uint64_t)t.tv_sec * UINT64_C(1000000000) + (uint64_t)t.tv_nsec;
}
static int cmp_ns(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}
static void run(uint32_t n, const uint8_t *data, const int reverse,
                const int rate, unsigned trial, unsigned blocks)
{
    breaks_t *b = calloc(1, sizeof(*b));
    uint64_t *durations = calloc(blocks, sizeof(*durations));
    if (!b || !durations) { fputs("allocation failed\n", stderr); exit(3); }
    brk_loop_t loop = {.adpcm = data, .nsamples = n, .rate = 22050, .bars = 2};
    breaks_init(b);
    breaks_set_loop(b, 0, &loop);
    if (!b->bank[0].valid) { fputs("break bank invalid\n", stderr); exit(1); }
    breaks_set(b, BRK_LEVEL, 127);
    breaks_set(b, BRK_PITCH, 12);
    if (reverse) breaks_live(b, 8, 1);
    float sp16 = (float)n / (32.0f * (float)rate);
    if (!(sp16 > 1.0f)) { fputs("bad playback setup\n", stderr); exit(3); }
    breaks_step(b, 0, 0, sp16, 1);
    uint64_t hash = UINT64_C(1469598103934665603), audible = 0;
    uint64_t sum = 0;
    float out[BLOCK];
    for (unsigned j = 0; j < blocks; ++j) {
        const uint64_t before = ns();
        breaks_render(b, out, (int)BLOCK);
        const uint64_t after = ns();
        durations[j] = after - before;
        sum += durations[j];
        for (unsigned i = 0; i < BLOCK; ++i) {
            uint32_t bits = 0;
            if (!isfinite(out[i]) || fabsf(out[i]) > 2.0f) {
                fprintf(stderr, "invalid PCM len=%" PRIu32 " rev=%d rate=%d block=%u frame=%u: %g\n",
                        n, reverse, rate, j, i, out[i]); exit(1);
            }
            audible += (out[i] != 0.0f);
            memcpy(&bits, &out[i], sizeof(bits));
            for (unsigned k = 0; k < sizeof(bits); ++k) {
                hash ^= (uint8_t)(bits >> (k * 8u));
                hash *= UINT64_C(1099511628211);
            }
        }
    }
    if (!audible) { fputs("unexpectedly silent renderer\n", stderr); exit(1); }
    if (reverse) breaks_live(b, 8, 0);
    breaks_stop(b); breaks_set_quiet(b, 1);
    for (int i=0;i<16;++i) breaks_render(b,out,(int)BLOCK);
    if (!breaks_silent(b)) { fputs("non-silent after stop\n", stderr); exit(1); }
    for (unsigned i=0;i<BLOCK;++i) if (out[i] != 0.0f) {
        fputs("nonzero after stop\n", stderr); exit(1);
    }
    qsort(durations, blocks, sizeof(*durations), cmp_ns);
    printf("n=%" PRIu32 " direction=%c rate=%d trial=%u blocks=%u frames=%u"
           " hash=%016" PRIx64 " audible=%" PRIu64 " p50=%" PRIu64
           " p95=%" PRIu64 " p99=%" PRIu64 " max=%" PRIu64
           " sum=%" PRIu64 " stopped_silent=1\n", n, reverse ? 'R' : 'F',
           rate, trial, blocks, blocks * BLOCK, hash, audible,
           durations[(blocks - 1u)/2u], durations[((blocks-1u)*95u)/100u],
           durations[((blocks-1u)*99u)/100u], durations[blocks - 1u], sum);
    free(durations); free(b);
}
int main(int argc, char **argv)
{
    if (argc != 3 || strcmp(argv[1], "--blocks") != 0) {
        fputs("usage: breaks_callback_tail_perf_test --blocks 64..4096\n", stderr);
        return 2;
    }
    char *end = NULL;
    unsigned long arg = strtoul(argv[2], &end, 10);
    if (!*argv[2] || *end || arg < MIN_BLOCKS || arg > MAX_BLOCKS) {
        fputs("blocks outside 64..4096\n", stderr);
        return 2;
    }
    unsigned blocks = (unsigned)arg;
    static const uint32_t lengths[] = {65535u, 2116800u};
    static const int rates[] = {1,4,16,32,64,128,256,512,1024,2048};
    for (unsigned l = 0; l < sizeof(lengths)/sizeof(lengths[0]); ++l) {
        uint32_t n = lengths[l], seed = UINT32_C(0x43a9d671) ^ n;
        uint8_t *data = malloc(((size_t)n + 1u)/2u);
        if (!data) { fputs("data allocation failed\n", stderr); return 3; }
        for (size_t i = 0; i < ((size_t)n + 1u)/2u; ++i) data[i]=(uint8_t)rng(&seed);
        for (int rev = 0;rev <= 1; ++rev) {
            for (unsigned r = 0; r < sizeof(rates)/sizeof(rates[0]); ++r) {
                if (((float)n / (32.0f * (float)rates[r])) <= 1.0f) continue;
                for (unsigned trial = 0; trial < 2; ++trial)
                    run(n, data, rev, rates[r], trial, blocks);
            }
        }
        free(data);
    }
    return 0;
}
