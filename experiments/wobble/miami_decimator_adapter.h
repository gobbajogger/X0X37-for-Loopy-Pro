/* SPDX-License-Identifier: GPL-3.0-only */
/* Inactive original-state projection. Private coefficient dependency. */
#ifndef MIAMI_DECIMATOR_ADAPTER_H
#define MIAMI_DECIMATOR_ADAPTER_H
#include "miami_decimator_component.h"
typedef struct {
    miami_decimator_component_t kernel;
    int16_t wet_gain;
    uint8_t frequency_integer, wet_integer, bit_integer;
} miami_decimator_adapter_t;
int miami_decimator_adapter_init(miami_decimator_adapter_t *s);
/* Selected-effect reset only: phase/held cleared, parameters/pending preserved. */
int miami_decimator_adapter_reset(miami_decimator_adapter_t *s);
/* Original integer parameters1frequency/2wet/3bit; value&127 as proven.
 * Bit changes staged frequency timing, but no bit-depth mask affects this path. */
int miami_decimator_adapter_set(miami_decimator_adapter_t *s, unsigned parameter, int32_t value);
/* One actual effect invocation: index restarts0, not global host sample position.
 * Buffers are disjoint in-place stereo arrays and must not overlap s/events.
 * reset_frames is a caller-supplied sorted list of sample-local reset events,
 * applied BEFORE latch/sample. This is NOT a parser for original event bytes or
 * host automation. All events checked before mutation; emptycall commits nothing.
 * No concurrent setter/process access is supported. */
int miami_decimator_adapter_process(miami_decimator_adapter_t *s, int16_t *left,
                                    int16_t *right, uint32_t frames,
                                    const uint32_t *reset_frames, uint32_t reset_count);
#endif
