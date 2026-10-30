/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_POW2_FMA_H
#define ORIGINAL_MIAMI_POW2_FMA_H
#include "binary32_math.h"
#include <math.h>
#ifdef ORIGINAL_USE_SOFT_FMA
#include "fused_reference.h"
#define MIAMI_POW2_FMA(a,b,c) original_rne_fmaf((a),(b),(c))
#else
#define MIAMI_POW2_FMA(a,b,c) fmaf((a),(b),(c))
#endif

/* Miami's selected MG waveforms restrict x to [-1,1]. Multiplying a finite
 * binary32 value in this range by 2 is exact, including subnormals. The
 * following addition is therefore the original FMA's sole rounding. */
static inline float original_miami_mg_twice_plus_one(float x)
{
    return x * 2.0f + 1.0f;
}

/* Multiplication by 1/2 is exact for |x| >= 2*FLT_MIN and for zero.
 * Small positive/negative normals and subnormals retain the fused reference;
 * never assume that a rounded underflowed product is an exact FMA operand.
 * This preserves the selected MG bipolar->unipolar mapping byte-for-byte. */
static inline float original_miami_mg_half_plus_half(float x)
{
    uint32_t magnitude = original_math_bits(x) & UINT32_C(0x7fffffff);
    if (magnitude == 0 || magnitude >= UINT32_C(0x01000000))
        return x * 0.5f + 0.5f;
    return MIAMI_POW2_FMA(x, 0.5f, 0.5f);
}
#undef MIAMI_POW2_FMA
#endif
