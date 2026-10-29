/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_STARTUP_H
#define ORIGINAL_MIAMI_STARTUP_H
#include <stdint.h>
enum {ORIGINAL_MIAMI_STARTUP_WRAPPERS=1,ORIGINAL_MIAMI_STARTUP_CORE,ORIGINAL_MIAMI_STARTUP_RESTORE,ORIGINAL_MIAMI_STARTUP_WARMUP,ORIGINAL_MIAMI_STARTUP_COMPLETE};
typedef struct {uint32_t target,rendered,blocks,stage;int status;} original_miami_startup_state;
typedef struct {
 void *context;
 int (*reset_both_wrappers)(void *);
 int (*reset_core)(void *);
 int (*restore)(void *,unsigned native_w1);
 int (*render)(void *,const uint32_t *,const uint32_t *,uint32_t *,uint32_t *,uint32_t frames);
} original_miami_startup_owner;
/* Actual7AAC4 scheduling seam for existing startup owner: both full wrapper
 * resets, coreB0A80, stateapply7C030(w1=0), then zero4096 scratch and render256
 * until accumulated>=trunc(float(Fs*.1f)). At44100:18 calls/4608 frames.
 * Supply1024 disjoint caller-owned uint32 words; no heap or stack audio arrays.
 * Each callback must implement the named original operation, including exact
 * all-owned-FX reset and core reset phase preservation. The selected fixed-FX
 * projection alone is not permission to claim a complete VST reset.
 * This implements ordering/warmup, not an invented cold/restored snapshot,
 * native-rate table generation, host resampler or existing storage adoption.
 * Validation precedes callbacks. A callback failure records its exact stage
 * and progress and returns0 after any earlier side effects; never auto-retries.
 * Core-only reset preserves master; constructor phases are a separate action.
 */
int original_miami_startup_reset(original_miami_startup_state *,const original_miami_startup_owner *,uint32_t *scratch1024,float native_sample_rate);
#endif
