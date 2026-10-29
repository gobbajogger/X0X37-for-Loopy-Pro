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
#endif
