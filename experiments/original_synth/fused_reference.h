/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_SYNTH_FUSED_REFERENCE_H
#define ORIGINAL_SYNTH_FUSED_REFERENCE_H
#include <stdint.h>
/* Independently written exact finite binary32 multiply-add, RNE ties-even.
 * Reference implementation for targets without hardware/library fmaf. No soft
 * double, no approximation. Returns0 on NaN/Inf inputs/null out, untouched out.
 * Overflow/underflow result bits follow RNE; exception flags are not emulated.
 * This is a correctness baseline, NOT a qualified real-time optimization. */
/* Retained full-range integer oracle/fallback; compact returns0 outside its
 * exact alignment domain without touching output. Neither uses soft double. */
int original_fma_finite_bits_reference(uint32_t a,uint32_t b,uint32_t c,uint32_t *out);
int original_fma_compact_bits(uint32_t a,uint32_t b,uint32_t c,uint32_t *out);
int original_fma_finite_bits(uint32_t a,uint32_t b,uint32_t c,uint32_t *out);
/* Finite-input adapter; unsupported nonfinite inputs return a quiet NaN. */
float original_rne_fmaf(float a,float b,float c);
/* X0X_FAST_FMA (device build): a multiply, then an add (two roundings) in place of the exact
 * software FMA. The AC79 has no FMA instruction. Measured difference: docs/perf/engine-load-20261031.md. */
#if defined(X0X_FAST_FMA) && !defined(ORIGINAL_FUSED_REFERENCE_IMPL) && !defined(X0X_EXACT_FMA_HERE)
static inline float original_fast_fmaf(float a,float b,float c){float p=a*b;return p+c;}
#define original_rne_fmaf(a,b,c) original_fast_fmaf((a),(b),(c))
#endif
#endif
