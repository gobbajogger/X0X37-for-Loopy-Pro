/* SPDX-License-Identifier: GPL-3.0-only */
/* Native-only IMA decoder correctness and reproducible workload profiler.
 * The production decoder is included unchanged. Timing is a HOST proxy,
 * never an SMK-37 cycle count or audio-XRUN measurement.
 *
 *   cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/ima_perf \
 *      tests/host/ima_decode_perf_test.c
 *   /tmp/ima_perf --check
 *   /tmp/ima_perf --bench
 *
 * For ASan/UBSan, use --check only: instrumentation distorts timings.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../firmware/src/dsp/ima_adpcm.h"

enum { DATA_BYTES = 8192, TRIALS = 9, PASSES = 96 };
static uint8_t data[DATA_BYTES];
static volatile uint64_t timing_sink;

/* Independent IMA step logic: do not call production ima_decode here. */
static int32_t reference_decode(const uint8_t *src, uint32_t nibble, int32_t *pred, int32_t *idx)
{
    static const int8_t index_adjust[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
    uint32_t code = (src[nibble / 2u] >> ((nibble % 2u) * 4u)) & 15u;
    int32_t step = IMA_STEP_T[*idx];
    int32_t delta = step / 8;
    if ((code / 4u) % 2u) delta += step;
    if ((code / 2u) % 2u) delta += step / 2;
    if (code % 2u) delta += step / 4;
    if (code >= 8u) delta = -delta;
    int32_t next = *pred + delta;
    if (next < -32768) next = -32768;
    if (next > 32767) next = 32767;
    *pred = next;
    next = *idx + index_adjust[code % 8u];
    if (next < 0) next = 0;
    if (next > 88) next = 88;
    *idx = next;
    return *pred;
}

static uint32_t random32(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *state = x;
}

static void fill(unsigned mode)
{
    uint32_t seed = 0x93d43fa5u;
    for (unsigned i = 0; i < DATA_BYTES; ++i) {
        uint8_t x = (uint8_t)random32(&seed);
        data[i] = mode == 0u ? x : mode == 1u ? 0x00u : mode == 2u ? 0xffu :
                  ((i & 1u) ? 0xf0u : 0x0fu);
    }
}

static int check(void)
{
    const int32_t preds[] = {-32768, -32767, -16384, -1, 0, 1, 16384, 32766, 32767};
    uint64_t cases = 0;
    if (IMA_STEP_T[0] != 7 || IMA_STEP_T[88] != 32767) return 1;
    for (int idx = 0; idx <= 88; ++idx) {
        for (uint32_t code = 0; code < 16; ++code) {
            uint8_t bytes[] = {(uint8_t)code, (uint8_t)(code << 4)};
            for (unsigned nibble = 0; nibble < 2; ++nibble) {
                /* For nibble=1, use the upper nibble in bytes[0]. */
                bytes[0] = nibble ? (uint8_t)(code << 4) : (uint8_t)code;
                for (unsigned j = 0; j < sizeof(preds) / sizeof(preds[0]); ++j) {
                    int32_t p = preds[j], k = idx, q = preds[j], r = idx;
                    int32_t a = ima_decode(bytes, nibble, &p, &k);
                    int32_t b = reference_decode(bytes, nibble, &q, &r);
                    if (a != b || p != q || k != r) {
                        fprintf(stderr, "FAIL isolated code=%u nibble=%u pred=%d idx=%d: %d/%d, %d/%d\n",
                                code, nibble, preds[j], idx, p, q, k, r);
                        return 1;
                    }
                    ++cases;
                }
            }
        }
    }
    for (unsigned mode = 0; mode < 4; ++mode) {
        int32_t p = 0, k = 0, q = 0, r = 0;
        fill(mode);
        for (uint32_t nibble = 0; nibble < DATA_BYTES * 2u; ++nibble) {
            int32_t a = ima_decode(data, nibble, &p, &k);
            int32_t b = reference_decode(data, nibble, &q, &r);
            if (a != b || p != q || k != r) {
                fprintf(stderr, "FAIL stream mode=%u nibble=%u: %d/%d, %d/%d\n",
                        mode, nibble, p, q, k, r);
                return 1;
            }
            ++cases;
        }
    }
    printf("CHECK PASS: %llu nibble comparisons, 89 step states, 16 codes, "
           "9 predictor boundaries, both packed nibble positions, 4 streams\n",
           (unsigned long long)cases);
    return 0;
}

#if defined(__GNUC__) || defined(__clang__)
#define NOINLINE __attribute__((noinline))
#else
#define NOINLINE
#endif
static NOINLINE uint64_t bench_decode(void)
{
    uint64_t sum = 0;
    int32_t p = 0, idx = 0;
    for (unsigned round = 0; round < PASSES; ++round) {
        p = idx = 0;
        for (uint32_t nibble = 0; nibble < DATA_BYTES * 2u; ++nibble)
            sum += (uint32_t)(ima_decode(data, nibble, &p, &idx) + 32768);
    }
    return sum + (uint32_t)idx;
}

static int monotonic_ns(uint64_t *ns)
{
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) != 0) return -1;
    *ns = (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec;
    return 0;
}

static int compare_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static int bench(void)
{
    static const char *const names[] = {"prng", "all-00", "all-ff", "alternating-0f-f0"};
    for (unsigned mode = 0; mode < 4; ++mode) {
        double samples[TRIALS], sorted[TRIALS], spread[TRIALS];
        uint64_t checksum = 0;
        fill(mode);
        timing_sink = bench_decode(); /* warm up code, pages and caches */
        for (int trial = 0; trial < TRIALS; ++trial) {
            uint64_t a, b;
            if (monotonic_ns(&a)) return 2;
            checksum = bench_decode();
            if (monotonic_ns(&b) || b < a) return 2;
            samples[trial] = (double)(b - a) / ((double)PASSES * DATA_BYTES * 2.0);
            sorted[trial] = samples[trial];
            timing_sink ^= checksum;
        }
        qsort(sorted, TRIALS, sizeof sorted[0], compare_double);
        for (int i = 0; i < TRIALS; ++i) {
            double d = samples[i] - sorted[TRIALS / 2];
            spread[i] = d < 0 ? -d : d;
        }
        qsort(spread, TRIALS, sizeof spread[0], compare_double);
        printf("BENCH workload=%s nibbles_per_trial=%u trials=%d "
               "median_ns=%.3f min_ns=%.3f max_ns=%.3f MAD_ns=%.3f checksum=%llu\n",
               names[mode], (unsigned)(PASSES * DATA_BYTES * 2u), TRIALS,
               sorted[TRIALS / 2], sorted[0], sorted[TRIALS - 1],
               spread[TRIALS / 2], (unsigned long long)checksum);
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2 || (strcmp(argv[1], "--check") && strcmp(argv[1], "--bench"))) {
        fprintf(stderr, "usage: %s --check|--bench\n", argv[0]);
        return 2;
    }
    if (check()) return 1;
    return strcmp(argv[1], "--bench") == 0 ? bench() : 0;
}
