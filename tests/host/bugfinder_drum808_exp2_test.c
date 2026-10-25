/* SPDX-License-Identifier: GPL-3.0-only
 * BF-006 host probe: invoke the production 808 high-precision 2^x helper.
 * An in-range negative exponent must not cause undefined signed-left-shift.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../firmware/src/dsp/drum808.c"

int main(void)
{
    const float x[] = {-3.0f, -1.0f, -0.5f, -12.0f, -64.0f, 0.0f, 1.0f, 12.0f, 64.0f};
    for (unsigned i = 0; i < sizeof x / sizeof x[0]; ++i) {
        float got = d8_exp2_cr(x[i]);
        float expected = exp2f(x[i]);
        if (!(isfinite(got) && got > 0.f && fabsf(got - expected) <= expected * 1.0e-6f)) {
            fprintf(stderr, "FAIL exponent=%g actual=%g expected=%g\n", x[i], got, expected);
            return 1;
        }
    }
    puts("PASS 808 2^x: nine numeric controls, no UB for negative exponents");
    return 0;
}
