/* SPDX-License-Identifier: GPL-3.0-only */
#include "miami_fx_conversion.h"
#include "../original_synth/binary32_math.h"
#include <float.h>
#ifndef MIAMI_FX_NO_HOST_FENV
#include <fenv.h>
#endif
#include <limits.h>
#include <math.h>
#include <string.h>
#ifdef ORIGINAL_USE_SOFT_FMA
#include "../original_synth/fused_reference.h"
#define ref_fma original_rne_fmaf
#else
#define ref_fma fmaf
#endif
#if FLT_RADIX != 2 || FLT_MANT_DIG != 24 || FLT_MAX_EXP != 128
#error IEEE binary32 float required
#endif
#if FLT_EVAL_METHOD != 0
#error Excess precision is outside the binary32 reference contract
#endif
#if defined(__FAST_MATH__)
#error Fast math is outside the reference arithmetic contract
#endif
typedef char float_is_32_bits[(sizeof(float)==sizeof(uint32_t))?1:-1];
static float from_bits(uint32_t u) { return original_math_float(u); }
static uint32_t bits(float x) { return original_math_bits(x); }
int miami_fx_rne_environment(void) {
#ifdef MIAMI_FX_NO_HOST_FENV
    return 0; /* No target FP environment proof. Never claim probe success. */
#else
    volatile float tiny=from_bits(1), one=1.0f;
    volatile float normal=from_bits(0x00800000u), half=0.5f;
    return fegetround()==FE_TONEAREST && bits(tiny*one)==1 &&
           bits(normal*half)==0x00400000u;
#endif
}
uint32_t miami_fx_cleanup_rne(uint32_t u) {
    uint32_t mag=u&0x7fffffffu;
    if (mag>0x7f800000u || mag<=0x3f000000u) return u;
    float z=from_bits(u), a,b,c;
    if (!(u>>31)) {
        if (z>1.0f) z=1.0f;
        a=ref_fma(z,2.0f,-1.0f); b=1.0f-a;
        c=ref_fma(-b,b,1.0f); return bits(ref_fma(c,0.25f,0.5f));
    }
    if (z< -1.0f) z=-1.0f;
    a=ref_fma(z,-2.0f,-1.0f); b=1.0f-a;
    c=ref_fma(-b,b,1.0f); return bits(ref_fma(-c,0.25f,-0.5f));
}
int miami_fx_scaled_rne(uint32_t u, unsigned fz, uint32_t *out) {
    if (!out || fz>1) return 0;
    uint32_t mag=u&0x7fffffffu;
    if (mag>0x7f800000u) { *out=u|0x00400000u; return 1; }
    if (fz && mag<0x00800000u) u&=0x80000000u;
    float scaled=from_bits(u)*32767.0f;
    uint32_t result=bits(scaled);
    if (fz && (result&0x7fffffffu)<0x00800000u) result&=0x80000000u;
    *out=result; return 1;
}
int32_t miami_fx_fcvtzs(uint32_t u) {
    uint32_t mag=u&0x7fffffffu, negative=u>>31;
    if (mag>0x7f800000u) return 0;
    if (mag>=0x4f000000u) return negative?INT32_MIN:INT32_MAX;
    if (mag<0x3f800000u) return 0;
    unsigned exponent=(mag>>23)-127u;
    uint32_t significand=(mag&0x007fffffu)|0x00800000u;
    uint32_t value=exponent>=23 ? significand<<(exponent-23) :
                                               significand>>(23-exponent);
    /* Above guards ensure value<=INT32_MAX before this conversion. */
    return negative?-(int32_t)value:(int32_t)value;
}
int16_t miami_fx_low16(int32_t input) {
    uint32_t low=(uint32_t)input&65535u;
    int32_t signed_low=low<32768u?(int32_t)low:(int32_t)low-65536;
    return (int16_t)signed_low;
}
int miami_fx_input_rne(uint32_t u, unsigned fz, int16_t *out) {
    if (!out || fz>1) return 0;
    /* Both binary32 zeros convert to integer zero in either FZ mode. Keep
     * this stateless fast path after validation; clocks and FX still run. */
    if ((u&0x7fffffffu)==0) { *out=0; return 1; }
    uint32_t scaled;
    (void)miami_fx_scaled_rne(miami_fx_cleanup_rne(u),fz,&scaled);
    *out=miami_fx_low16(miami_fx_fcvtzs(scaled)); return 1;
}
uint32_t miami_fx_output_rne(int16_t input) {
    uint32_t magnitude=input<0?(uint32_t)(-(int32_t)input):(uint32_t)input;
    if (!magnitude) return 0;
    unsigned top=31u-(unsigned)__builtin_clz(magnitude);
    uint32_t significand=magnitude<<(23u-top);
    /* Exact RNE division by 2^15-1. With the numerator normalized to
     * 24 bits, the result is A + round(A/32767). Splitting A at bit15
     * leaves a remainder below 32767+512; its half-denominator test
     * also covers the single possible quotient carry. The denominator
     * is odd, so no halfway case exists. No reciprocal approximation. */
    uint32_t quotient=significand>>15;
    uint32_t remainder=(significand&32767u)+quotient;
    significand+=quotient+(remainder>16383u);
    uint32_t exponent=top+112u;
    if (significand==0x01000000u) {
        significand>>=1;
        ++exponent;
    }
    return (input<0?0x80000000u:0u)|(exponent<<23)|(significand&0x007fffffu);
}
