/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef MIAMI_FX_CONVERSION_H
#define MIAMI_FX_CONVERSION_H
#include <stdint.h>
/* Inactive reference primitives, NOT an active FX wrapper.
 * Host prerequisites: IEEE binary32, round-to-nearest ties-even, gradual
 * underflow, masked FP exceptions; no fast-math/reciprocal substitution.
 * Probe detects rounding/gradual-underflow only, NOT enabled exception traps.
 * Values emulate evidenced ARM RNE/DN0 results, not FP flags or trap behavior.
 * Live original host FPCR is unknown. No other rounding/DN modes supported. */
int miami_fx_rne_environment(void);
/* Bit interfaces preserve signed zero and cleanup NaN payload without requiring
 * host signaling-NaN arithmetic. No DSP memory or global mutable state. */
uint32_t miami_fx_cleanup_rne(uint32_t input_bits);
/* fz is explicit original FMUL input/output flushing: 0 or 1 only.
 * Returns0 on invalid fz/null output, without modifying output. */
int miami_fx_scaled_rne(uint32_t cleaned_bits, unsigned fz, uint32_t *out_bits);
/* Instruction-result emulation, all binary32 bit patterns, no undefined cast. */
int32_t miami_fx_fcvtzs(uint32_t scaled_bits);
int16_t miami_fx_low16(int32_t input);
int miami_fx_input_rne(uint32_t input_bits, unsigned fz, int16_t *out);
/* Exact signed16 domain, includes -32768. FDIV32767, NOT reciprocal FMUL. */
uint32_t miami_fx_output_rne(int16_t input);
/* Same result as miami_fx_cleanup_rne for every input: it returns |x|<=0.5
 * and NaN unchanged, so only the rest calls the full cleanup. */
static inline uint32_t miami_fx_cleanup_fast(uint32_t u){
    uint32_t mag=u&0x7fffffffu;
    return (mag>0x3f000000u&&mag<=0x7f800000u)?miami_fx_cleanup_rne(u):u;
}
/* Same result as fcvtzs(scaled_rne(cleanup_rne(u),fz)) for every input and
 * either fz (checked for all 2^32 inputs): NaN gives 0; after cleanup |x|<=1,
 * so x*32767 is in range and the C truncating cast equals FCVTZS; a
 * subnormal, flushed or not, truncates to 0. */
static inline int32_t miami_fx_input_fast(uint32_t u){
    uint32_t mag=u&0x7fffffffu;float f;
    if(mag>0x7f800000u)return 0;
    if(mag>0x3f000000u)u=miami_fx_cleanup_rne(u);
    __builtin_memcpy(&f,&u,4);
    return (int32_t)(f*32767.0f);
}
/* miami_fx_output_rne, inline for the per-sample output loops (same code). */
static inline uint32_t miami_fx_output_rne_inline(int16_t input) {
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
#endif
