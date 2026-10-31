/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_SYNTH_FIXED_FX_H
#define ORIGINAL_SYNTH_FIXED_FX_H
#include <stdint.h>
#define ORIGINAL_FX_CAPACITY 2048u
typedef struct {
 int32_t target, gain_state;
 uint8_t active_enable,pending_enable,dirty;
} original_fixed_fx;
typedef void (*original_effect_process)(void *,int16_t *,int16_t *,uint32_t);
typedef void (*original_effect_reset_all)(void *);
void original_fixed_fx_init(original_fixed_fx *s);
/* Original parameter16 ignores its supplied value and queues ON. */
void original_fixed_fx_parameter16(original_fixed_fx *s);
/* Explicit internal method; active original Miami caller remains unproved.
 * This is NOT a proposed exposed bypass control. Sequential caller ownership. */
void original_fixed_fx_internal_enable(original_fixed_fx *s,uint8_t value);
/* Full reset forces target/gain up, preserves enable flags, resets ALL owned
 * effects through caller callback. Callback must implement that whole set. */
void original_fixed_fx_reset(original_fixed_fx *s,original_effect_reset_all reset,void *context);
/* Fixed selected algorithm only; excludes switch/outer gating/countdown policy.
 * Scratch L/R each hold n signed16 samples. All buffers must be disjoint,
 * except outL==dryL and/or outR==dryR. Callback processes scratch even disabled,
 * and on inner n==0. All capacity/null validation precedes state mutation.
 * Callback is total/infallible for validated frames; no concurrent setters. */
int original_fixed_fx_process(original_fixed_fx *s,const int32_t *dryL,const int32_t *dryR,
 int16_t *scratchL,int16_t *scratchR,int32_t *outL,int32_t *outR,uint32_t n,
 original_effect_process effect,void *context);
#endif
