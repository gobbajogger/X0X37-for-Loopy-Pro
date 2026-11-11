/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_BINARY32_CLASSIFY_H
#define ORIGINAL_BINARY32_CLASSIFY_H
#include <float.h>
#include <stdint.h>
#include <string.h>
#if FLT_RADIX != 2 || FLT_MANT_DIG != 24 || FLT_MAX_EXP != 128
#error Original engine requires IEEE binary32 float
#endif
/* The pinned SDK isfinite macro dispatches to __fpclassifyf in libc.a, which
 * is outside the accepted application link. Classification needs no floating
 * operation: both infinities and every quiet/signalling NaN have exponent255.
 * This accepts both zeros, all subnormals and every finite normal. The bit
 * predicate itself is quiet; a caller's subsequent floating range comparison
 * may be hoisted before it by the target compiler. Rejected nonfinite inputs
 * return failure without state/output writes. Floating exception flags for
 * those out-of-domain inputs are not a parity guarantee. In particular, do
 * not infer whole-call signalling-NaN FP status from this integer predicate.
 * Target instruction/FP-status qualification remains separate from host
 * return/state parity. No FP environment is read, cleared or changed here. */
static inline int original_binary32_isfinite(float value){
 uint32_t bits;typedef char original_float_size[(sizeof value==sizeof bits)?1:-1];
 (void)sizeof(original_float_size);
#ifdef __cplusplus
 /* C++ has different inactive-union-member rules. */
 memcpy(&bits,&value,sizeof bits);
#else
 /* C99 union representation access is intentional: uint32_t has no trap or
  * padding representation. This is the same four bytes as memcpy, while the
  * pinned C target can lower it without a four-byte library call. Target
  * instruction/cost confirmation remains required; no speedup is assumed. */
 union {float value;uint32_t bits;} representation;
 representation.value=value;bits=representation.bits;
#endif
 return (bits&UINT32_C(0x7f800000))!=UINT32_C(0x7f800000);
}
#endif
