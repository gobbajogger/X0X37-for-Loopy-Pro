/* SPDX-License-Identifier: GPL-3.0-only */
/* Exponent boundaries and exact binary scaling; also emits a legacy parity trace. */
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/dsp/fastmath.h"

static unsigned checks, failures;
static uint64_t digest = UINT64_C(14695981039346656037);
static uint32_t bits(float x)
{
    uint32_t u;
    memcpy(&u, &x, sizeof(u));
    return u;
}
static void check(float actual, float expected)
{
    uint32_t u = bits(actual);
    checks++;
    digest = (digest ^ u) * UINT64_C(1099511628211);
    if (u != bits(expected)) {
        fprintf(stderr, "check %u: %08" PRIx32 " != %08" PRIx32 "\n",
                checks, u, bits(expected));
        failures++;
    }
}
int main(void)
{
    /* Independent scaling oracle: ldexpf, with the existing polynomial at n=0.
     * Exclude below-normal helper combinations unused by the public wrappers. */
    for (int n = -126; n <= 128; n++) {
        for (int k = -16; k <= 16; k++) {
            float f = (float)k / 32.0f;
            if ((n == -126 && k < 0) || (n == 128 && k >= -1))
                continue;
            check(fm_exp2_nf(n, f), ldexpf(fm_exp2_nf(0, f), n));
        }
    }
    check(fm_exp2_nf(INT_MIN, 0.0f), 0.0f);
    check(fm_exp2_nf(-127, 0.5f), 0.0f);
    check(fm_exp2f(nextafterf(-126.0f, -INFINITY)), 0.0f);
    check(fm_exp2f(-126.0f), 0x1p-126f);
    check(fm_exp2f(0.0f), 1.0f);
    check(fm_exp2f(127.0f), 0x1p127f);
    check(fm_exp2f(nextafterf(127.0f, INFINITY)), fm_exp2f(127.0f));
    check(fm_expf(nextafterf(-87.3f, -INFINITY)), 0.0f);
    check(fm_expf(nextafterf(88.7f, INFINITY)), fm_expf(88.7f));
    /* Adjacent binary32 values around every exp2 rounding transition. */
    for (int n = -125; n < 127; n++) {
        float midpoint = (float)n + 0.5f;
        for (int k = -1; k <= 1; k++) {
            float x = k < 0 ? nextafterf(midpoint, -INFINITY) :
                      k > 0 ? nextafterf(midpoint, INFINITY) : midpoint;
            float actual = fm_exp2f(x);
            double reference = exp2((double)x);
            if (!isfinite(actual) || fabs((double)actual - reference) / reference > 3e-7)
                failures++;
            check(actual, actual); /* Include wrapper output in legacy parity digest. */
        }
    }
    printf("fastmath exponent: %u checks, %u failures, bit digest %016" PRIx64 "\n",
           checks, failures, digest);
    return failures ? 1 : 0;
}
