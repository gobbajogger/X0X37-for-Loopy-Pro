/* SPDX-License-Identifier: GPL-3.0-only */
/* Inactive, independently written component from sanitized arithmetic evidence.
 * No coefficient tables, macro mapping, float wrapper or original reset defaults. */
#ifndef MIAMI_DECIMATOR_COMPONENT_H
#define MIAMI_DECIMATOR_COMPONENT_H
#include <stdint.h>
typedef struct {
    uint16_t phase, step, pending_step;
    int16_t held_left, held_right;
    uint8_t pending;
} miami_decimator_component_t;
/* Explicit caller-provided state, not an emulation of original reset defaults.
 * Checked failure leaves the destination untouched. Held values exclude -32768. */
int miami_decimator_seed(miami_decimator_component_t *s, uint32_t phase,
                         int32_t held_left, int32_t held_right, uint32_t step);
/* Stage explicit step; latest pending value wins at a supplied index divisible64.
 * This does not map original Frequency integers; tables are unavailable. */
int miami_decimator_stage_step(miami_decimator_component_t *s, uint32_t step);
/* Caller supplies original-local sample index; no global timing is inferred.
 * Explicit wet gain0..32767, not a normalized macro/table index. Wet-setter timing
 * is outside this component. Stereo input is by value, output is exactly2samples.
 * s and out must not overlap. Both are caller-owned; no allocation/global state.
 * Failure leaves state and output untouched. A seed is required before processing. */
int miami_decimator_frame(miami_decimator_component_t *s, uint64_t local_index,
                          int16_t left, int16_t right, int32_t wet, int16_t out[2]);
/* Exact event-free zero-input invocation starting at local index0. Returns0
 * without mutation unless both held samples are zero. Pending step commits
 * at index0 of a nonempty invocation; phase wraps exactly modulo65536. */
int miami_decimator_advance_zero(miami_decimator_component_t *s,uint32_t frames);
#endif
