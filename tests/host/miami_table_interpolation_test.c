/* SPDX-License-Identifier: GPL-3.0-only */
/* Miami selected filter-coefficient / resonance table interpolation.
 * No private #684 table data: test the public IEEE-binary32 lookup domain
 * against the independent complete 18-limb fused multiply-add oracle. */
#include "miami_interpolation.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static uint32_t rng = UINT32_C(0x4d694d20);
static uint32_t random_bits(void) {
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return rng;
}
static unsigned long checks, guarded, fallback;
static void compare(float difference,float fraction,float base) {
    uint32_t a = original_math_bits(base), d = original_math_bits(difference), f = original_math_bits(fraction), oracle;
    if (!original_fma_finite_bits_reference(d,f,a,&oracle)) {
        fprintf(stderr, "invalid oracle operands: %08x %08x %08x\n",d,f,a); exit(1);
    }
    uint32_t got = original_math_bits(original_miami_interpolate_rne(difference,fraction,base));
    if (got != oracle) {
        fprintf(stderr, "table FMA mismatch at %lu: %08x*%08x+%08x => %08x, oracle %08x\n",checks,d,f,a,got,oracle);
        exit(1);
    }
    unsigned ea = a >> 23, ed = (d >> 23) & 255u, ef = f >> 23;
    if (ea >= 2 && ea < 255 && (d >> 31) && ed && ed + 1 < ea && ef < 127) guarded++;
    else fallback++;
    ++checks;
}
/* Exact public MS20 lookup input domain: interior q is positive finite;
 * fraction is computed from the actual rounded binary32 q, not an invented
 * higher-precision coordinate. Resonance lookup has the same domain. */
static void check_table(const float table[129],float q) {
    if (q <= original_math_float(UINT32_C(0x3727c5ac)) || q >= 128) return;
    unsigned j = (unsigned)q;
    if (j >= 128) abort();
    compare(table[j + 1] - table[j], q - (float)j, table[j]);
}
static void fill_table(float table[129],unsigned mode) {
    for (unsigned j = 0; j < 129; j++) {
        switch (mode) {
        case 0: table[j] = 0.75f - (float)j * 0.000025f; break; /* positive/down, guard */
        case 1: table[j] = 0.0625f + (float)j * 0.000025f; break; /* positive/up */
        case 2: table[j] = -0.75f + (float)j * 0.000025f; break; /* negative */
        case 3: table[j] = original_math_float((j % 3 == 0) ? 1u : (UINT32_C(0x00800000) + j * 7u)); break;
        case 4: table[j] = original_math_float((j & 1u) ? UINT32_C(0x80000000) : 0u); break;
        default: {
            uint32_t x = random_bits(), e = 34u + ((x >> 24) % 154u);
            table[j] = original_math_float((x & UINT32_C(0x807fffff)) | (e << 23));
            break;
        }
        }
    }
}
int main(void) {
    float table[129];
    static const uint32_t fractions[] = {
        0u, 1u, 0x007fffffu, 0x00800000u, 0x33000000u,
        0x3a83126fu, 0x3d000000u, 0x3e000000u, 0x3f000000u,
        0x3f400000u, 0x3f7fffffu
    };
    /* Exhaust all 128 intervals at each deliberately varied table domain,
     * and exercise exact integer, subnormal, binade, and near-next-index q. */
    for (unsigned mode = 0; mode < 6; ++mode) {
        fill_table(table,mode);
        for (unsigned j = 0; j < 128; ++j) {
            for (unsigned k = 0; k < sizeof fractions / sizeof fractions[0]; ++k) {
                float q = (float)j + original_math_float(fractions[k]);
                check_table(table,q);
            }
        }
    }
    /* Dynamic filter and resonance coefficients: no table monotonicity
     * assumption. q is rounded before truncating and taking the difference. */
    for (unsigned i = 0; i < 400000u; ++i) {
        uint32_t a = random_bits();
        unsigned mode = i % 6u, j = (a >> 25) & 127u;
        float frac = original_math_float(UINT32_C(0x3f000000) | (a & UINT32_C(0x007fffff)));
        fill_table(table,mode);
        check_table(table,(float)j + frac);
    }
    if (guarded < 10000u || fallback < 10000u || checks < 400000u) {
        fprintf(stderr,"insufficient coverage: %lu checks %lu guarded %lu fallback\n",checks,guarded,fallback);
        return 1;
    }
    printf("PASS Miami table interpolation: %lu exact 18-limb cases (%lu guarded, %lu fallback)\n",checks,guarded,fallback);
    return 0;
}
