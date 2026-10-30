/* SPDX-License-Identifier: GPL-3.0-only */
/* Exact power-of-two selected Miami MG arithmetic: independent fmaf oracle. */
#include "miami_pow2_fma.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define CHECK(e) do { if (!(e)) { fprintf(stderr, "Miami power-of-two FMA test failed line %d: %s\n", __LINE__, #e); return 1; } } while (0)
static uint32_t rng = UINT32_C(0x68473123);
static uint32_t random_bits(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
#ifdef ORIGINAL_USE_SOFT_FMA
/* In the isolated test only, the soft fallback is backed by the correctly
 * rounded native oracle. The complete synthesizer regression checks the real
 * software 18-limb implementation separately. */
float original_rne_fmaf(float a,float b,float c) {return fmaf(a,b,c);}
#endif
static int check_phase(float phase)
{
    float twice = original_miami_mg_twice_plus_one(phase);
    float half = original_miami_mg_half_plus_half(phase);
    float t_reference = fmaf(phase, 2.0f, 1.0f);
    float h_reference = fmaf(phase, 0.5f, 0.5f);
    if (original_math_bits(twice) != original_math_bits(t_reference) ||
        original_math_bits(half) != original_math_bits(h_reference)) {
        fprintf(stderr, "mismatch input=%08x twice=%08x/%08x half=%08x/%08x\n",
            original_math_bits(phase),original_math_bits(twice),original_math_bits(t_reference),
            original_math_bits(half),original_math_bits(h_reference));
        return 0;
    }
    return 1;
}
int main(void)
{
    const uint32_t boundaries[] = {
        0,0x80000000u,1u,0x80000001u,0x007fffffu,0x807fffffu,
        0x00800000u,0x80800000u,0x00ffffffu,0x80ffffffu,
        0x01000000u,0x81000000u,0x01000001u,0x81000001u,
        0x3effffffu,0xbeffffffu,0x3f000000u,0xbf000000u,
        0x3f000001u,0xbf000001u,0x3f7fffffu,0xbf7fffffu,
        0x3f800000u,0xbf800000u
    };
    for (unsigned i = 0; i < sizeof boundaries / sizeof boundaries[0]; ++i)
        CHECK(check_phase(original_math_float(boundaries[i])));
    unsigned checked = 0;
    for (unsigned i = 0; i < 6000000u; ++i) {
        uint32_t bits = random_bits();
        if ((bits & UINT32_C(0x7fffffff)) > UINT32_C(0x3f800000))
            bits = (bits & UINT32_C(0x80000000)) | (bits & UINT32_C(0x007fffff));
        CHECK(check_phase(original_math_float(bits)));
        checked++;
    }
    /* Reference constants replace only provably constant original FMA calls.
     * These are used by the selected VCF envelope control graph. */
    CHECK(original_math_bits(fmaf(0.0f,13.0f,0.0010000000474974513f)) == UINT32_C(0x3a83126f));
    CHECK(original_math_bits(fmaf(0.0625f,25.0f,0.0010000000474974513f)) == UINT32_C(0x3fc820c5));
    CHECK(original_math_bits(fmaf(0.0f,25.0f,0.0020000000949949026f)) == UINT32_C(0x3b03126f));
    printf("PASS Miami power-of-two MG FMA and constant VCF ticks: %u random + %zu boundary probes\n",
           checked, sizeof boundaries / sizeof boundaries[0]);
    return 0;
}
