/* SPDX-License-Identifier: GPL-3.0-only */
/* Direct tanh entries against the packed table, and d9_tanh against the
 * a06e20189 two-lookup body for every non-NaN binary32 input (bit for bit). */
#define X0X_DRUM_TABLES_DEFINE
#include "drum909_dsp.h"
#include <stdio.h>
#include <string.h>

/* Original interpolation from the recut base, retained as an independent oracle. */
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
    for (unsigned i = 0; i < X0X_TANH_N + 2u; ++i) {
        if (bits(x0x_tanh_raw[i]) != bits(x0x_tanh_value(i))) {
            fprintf(stderr, "table mismatch at %u\n", i);
            return 1;
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
    printf("PASS: %u entries, %llu non-NaN binary32 inputs bit-identical\n",
           (unsigned)X0X_TANH_N + 2u, n);
    return 0;
}
