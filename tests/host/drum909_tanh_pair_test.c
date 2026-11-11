/* SPDX-License-Identifier: GPL-3.0-only */
/* d9_tanh_pair against x0x_tanh_value: every table index, and d9_tanh against
 * the two-lookup body for every non-NaN binary32 input (bit for bit). */
#define X0X_DRUM_TABLES_DEFINE
#include "drum909_dsp.h"
#include <stdio.h>
#include <string.h>

static float ref_tanh(float u)
{
    const float a = fm_fabsf(u) * X0X_TANH_SCALE;
    float t;
    if (a >= (float)X0X_TANH_N) {
        t = 1.0f;
    } else {
        const int32_t i = (int32_t)a;
        const float fr = a - (float)i;
        const float t0 = x0x_tanh_value((unsigned)i);
        const float t1 = x0x_tanh_value((unsigned)i + 1u);
        t = t0 + (t1 - t0) * fr;
    }
    return u < 0.0f ? -t : t;
}

static uint32_t bits(float f) { uint32_t u; memcpy(&u, &f, sizeof u); return u; }

int main(void)
{
    /* every index; and the 8-byte read of the packed path stays inside the words */
    for (unsigned i = 0; i < X0X_TANH_N; ++i) {
        float a, b;
        d9_tanh_pair(i, &a, &b);
        if (bits(a) != bits(x0x_tanh_value(i)) || bits(b) != bits(x0x_tanh_value(i + 1u))) {
            fprintf(stderr, "pair mismatch at %u\n", i);
            return 1;
        }
        unsigned r = 0, start = 0;
        while (i >= x0x_tanh_run_end[r]) { start = x0x_tanh_run_end[r]; ++r; }
        const x0x_tanh_run_t *run = &x0x_tanh_runs[r];
        if (i + 1u < x0x_tanh_run_end[r] && run->width && 2u * run->width <= 57u) {
            const unsigned byte = run->offset * 4u + (i - start) * run->width / 8u;
            if (byte + 8u > sizeof x0x_tanh_words) {
                fprintf(stderr, "read past the words at %u\n", i);
                return 1;
            }
        }
    }
    unsigned long long n = 0;
    uint32_t u = 0;
    do {
        float x;
        memcpy(&x, &u, sizeof x);
        if (x == x) {
            if (bits(d9_tanh(x)) != bits(ref_tanh(x))) {
                fprintf(stderr, "d9_tanh mismatch at 0x%08x\n", u);
                return 1;
            }
            ++n;
        }
    } while (++u != 0);
    printf("PASS: %u indices, %llu non-NaN binary32 inputs bit-identical\n", (unsigned)X0X_TANH_N, n);
    return 0;
}
