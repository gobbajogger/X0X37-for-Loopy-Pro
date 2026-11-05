/* SPDX-License-Identifier: GPL-3.0-only */
/* Regression for the compact IMA delta correction in shared BREAK/909 decoder.
 * Native host verification only; target ISR latency is not measured here. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../firmware/src/dsp/ima_adpcm.h"

static uint32_t rand32(uint32_t *seed)
{
    uint32_t x = *seed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *seed = x;
}

/* Independent sequential ADPCM reference; avoids LUT approximation. */
static int32_t ref_step(const uint8_t *d, uint32_t pos, int32_t *pred, int32_t *idx)
{
    static const int8_t delta_index[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
    uint32_t code = (d[pos / 2u] >> ((pos % 2u) * 4u)) & 15u;
    int32_t step = IMA_STEP_T[*idx];
    int32_t diff = step / 8;
    if (code & 4u) diff += step;
    if (code & 2u) diff += step / 2;
    if (code & 1u) diff += step / 4;
    int32_t v = *pred + (code & 8u ? -diff : diff);
    if (v < -32768) v = -32768;
    if (v > 32767) v = 32767;
    *pred = v;
    v = *idx + delta_index[code & 7u];
    if (v < 0) v = 0;
    if (v > 88) v = 88;
    *idx = v;
    return *pred;
}

static int check(void)
{
    static const int32_t boundary[] = {-32768, -32767, -16384, -1, 0, 1, 16384, 32766, 32767};
    static const uint32_t lengths[] = {64u, 65u, 511u, 512u, 513u, 122880u, 220500u, 705600u, 2116800u};
    uint64_t cases = 0;
    for (int k = 0; k <= 88; k++) {
        int32_t step = IMA_STEP_T[k];
        for (uint32_t mag = 0; mag < 8u; mag++) {
            int32_t expected = step / 8 + ((mag & 4u) ? step : 0)
                + ((mag & 2u) ? step / 2 : 0)
                + ((mag & 1u) ? step / 4 : 0);
            int32_t actual = (step * (int32_t)(2u * mag + 1u) >> 3)
                - IMA_ROUND_CORR[((step & 7) << 3) | mag];
            if (expected != actual) {
                fprintf(stderr, "delta mismatch step_idx=%d mag=%u ref=%d got=%d\n", k, mag, expected, actual);
                return 1;
            }
            cases++;
        }
        for (uint32_t nibble = 0; nibble < 2u; nibble++) {
            for (uint32_t code = 0; code < 16u; code++) {
                uint8_t d[1] = {(uint8_t)(code << (nibble * 4u))};
                for (unsigned j = 0; j < sizeof boundary / sizeof boundary[0]; j++) {
                    int32_t p = boundary[j], i = k, q = p, r = k;
                    int32_t got = ima_decode(d, nibble, &p, &i);
                    int32_t want = ref_step(d, nibble, &q, &r);
                    if (got != want || p != q || i != r) {
                        fprintf(stderr, "step mismatch idx=%d nibble=%u code=%u boundary=%d\n", k, nibble, code, boundary[j]);
                        return 1;
                    }
                    cases++;
                }
            }
        }
    }
    for (unsigned lenidx = 0; lenidx < sizeof lengths / sizeof lengths[0]; lenidx++) {
        uint32_t n = lengths[lenidx];
        uint8_t *d = malloc((n + 1u) / 2u);
        if (!d) return 3;
        uint32_t seed = 0x5a111f07u ^ n;
        for (uint32_t i = 0; i < (n + 1u) / 2u; i++) d[i] = (uint8_t)rand32(&seed);
        int32_t p = 0, i = 0, q = 0, r = 0;
        for (uint32_t pos = 0; pos < n; pos++) {
            int32_t got = ima_decode(d, pos, &p, &i), want = ref_step(d, pos, &q, &r);
            if (got != want || p != q || i != r) {
                fprintf(stderr, "stream mismatch length=%u sample=%u\n", n, pos);
                free(d);
                return 1;
            }
            cases++;
        }
        free(d);
    }
    printf("PASS: %llu exact decoder/reference and LUT algebra comparisons across 9 loops\n", (unsigned long long)cases);
    return 0;
}
int main(int argc, char **argv)
{
    if (argc != 2 || strcmp(argv[1], "--check")) {
        fputs("usage: ima_delta_lut_exact_test --check\n", stderr);
        return 2;
    }
    return check();
}
