/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_FX2_COMPACT_H
#define ORIGINAL_CHICAGO_FX2_COMPACT_H
#include "chicago_fx2_wrapper.h"
#include "chicago_modulation.h"
#include "chicago_dynamics.h"
#include "chicago_smallfx.h"

enum {
    ORIGINAL_CHICAGO_FX2_COMPACT_TYPES = 7,
    ORIGINAL_CHICAGO_FX2_COMPACT_HISTORY_SAMPLES = 2048
};

typedef union {
    original_chicago_chorus chorus;
    original_chicago_phaser phaser;
    original_chicago_ring ring;
    original_chicago_mastercomp mastercomp;
    original_chicago_decimator decimator;
    original_chicago_peaking peaking;
} original_chicago_fx2_compact_state;

/* One stable, caller-owned object per part. The selected scalar union is 96 B
 * on pi32v2; the stereo Chorus arena is 4096 B. Every part owns disjoint storage.
 * No heap, global scratch, history compression or reduced control range.
 * The original all 30/shared-arena owner cannot be obtained from this object. */
typedef struct {
    original_chicago_fx2_compact_state state;
    int16_t history[ORIGINAL_CHICAGO_FX2_COMPACT_HISTORY_SAMPLES];
    original_chicago_fx2_kernel kernel;
    original_mod_queue queue;
    uint32_t ready;
    uint8_t selected_type, selected;
} original_chicago_fx2_compact;

/* Initialize once before borrowing the owner. Do not move/copy a live object.
 * A later factory adoption uses backend_reset, which discards the selection
 * and queue before constructing the next requested original algorithm. */
int original_chicago_fx2_compact_init(original_chicago_fx2_compact *);
int original_chicago_fx2_compact_supports(unsigned original_type);
original_chicago_fx2_cold_owner original_chicago_fx2_compact_owner(original_chicago_fx2_compact *);

/* Immutable, sorted, unsigned EFFECT-INVOCATION LOCAL offsets after the real
 * owner's retiming. The stream is separate from backend engine events. Bind
 * before its actual FX invocation and retain records until consumed/rebound.
 * Binding starts cursor 0. Kernels keep their exact event consumption and
 * local index restart. Cold selection carries only the unconsumed stream;
 * it never replays consumed events. Bypass consumes no effect events.
 * Edits, tempo and direct/reset calls preserve the cursor. discard releases
 * the borrow. Invalid input leaves object and selected kernel untouched. */
int original_chicago_fx2_compact_events(original_chicago_fx2_compact *,
    const original_chicago_drive_event *,uint32_t count);
/* Original direct callbacks, for the selected object only. Type 5 resets the
 * selected kernel; type 2 also resets Chorus. This is not a cold selection. */
void original_chicago_fx2_compact_direct_event(original_chicago_fx2_compact *,unsigned type);
#endif
