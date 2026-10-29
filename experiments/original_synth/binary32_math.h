/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_BINARY32_MATH_H
#define ORIGINAL_BINARY32_MATH_H
#include <stdint.h>
#include <string.h>
static inline uint32_t original_math_bits(float x){
#ifdef __cplusplus
 uint32_t u;memcpy(&u,&x,4);return u;
#else
 union {float f;uint32_t u;} v;v.f=x;return v.u;
#endif
}
static inline float original_math_float(uint32_t x){
#ifdef __cplusplus
 float f;memcpy(&f,&x,4);return f;
#else
 union {uint32_t u;float f;} v;v.u=x;return v.f;
#endif
}
/* The replaced call sites already have finite operands. These operations
 * preserve every finite result bit including signed zero, without libm calls.
 * Floating exception status remains a separate, unqualified target contract. */
static inline float original_binary32_abs(float x){return original_math_float(original_math_bits(x)&0x7fffffffu);}
static inline float original_binary32_ceil_finite(float x){
 uint32_t u=original_math_bits(x),e=(u>>23)&255u,m=u&0x7fffffffu;
 if(e>=150u||!m)return x;
 if(e<127u)return original_math_float((u&0x80000000u)?0x80000000u:0x3f800000u);
 uint32_t mask=(1u<<(150u-e))-1u;
 if(!(u&mask))return x;
 if(!(u&0x80000000u))u+=mask;
 return original_math_float(u&~mask);
}
static inline float original_binary32_min_one_finite(float x){return x<1.0f?x:1.0f;}
#endif
