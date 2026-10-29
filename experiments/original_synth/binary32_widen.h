/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_BINARY32_WIDEN_H
#define ORIGINAL_BINARY32_WIDEN_H
#include <float.h>
#include <stdint.h>
#include <string.h>
#if FLT_RADIX != 2 || FLT_MANT_DIG != 24 || FLT_MAX_EXP != 128 || DBL_MANT_DIG != 53 || DBL_MAX_EXP != 1024
#error Original conversion requires IEEE binary32 and binary64
#endif
typedef char original_widen_sizes[(sizeof(float)==4&&sizeof(double)==8)?1:-1];
/* Exact representation widening of an ALREADY VALIDATED finite binary32.
 * No rounding, FP arithmetic or float-to-double compiler runtime is required.
 * Preserve signed zero and every subnormal. This is not a NaN/status policy.
 * C99 permits union representation access; C++ uses memcpy instead. The target
 * compiler's actual dependency/code cost still requires native measurement. */
static inline double original_binary32_to_binary64_finite(float value){
 uint32_t u;
#ifdef __cplusplus
 memcpy(&u,&value,4);
#else
 union {float f;uint32_t u;} input;input.f=value;u=input.u;
#endif
 uint32_t exponent=(u>>23)&255u,mantissa=u&0x7fffffu;
 uint64_t bits=(uint64_t)(u&0x80000000u)<<32;
 if(exponent){
  bits|=(uint64_t)(exponent+896u)<<52;
  bits|=(uint64_t)mantissa<<29;
 }else if(mantissa){
  exponent=897u;
  while(!(mantissa&0x800000u)){mantissa<<=1;exponent--;}
  bits|=(uint64_t)exponent<<52;
  bits|=(uint64_t)(mantissa&0x7fffffu)<<29;
 }
#ifdef __cplusplus
 double out;memcpy(&out,&bits,8);return out;
#else
 union {uint64_t u;double d;} output;output.u=bits;return output.d;
#endif
}
#endif
