/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_SMALLFX_H
#define ORIGINAL_CHICAGO_SMALLFX_H
#include "chicago_dynamics.h"
#include "miami_decimator_adapter.h"
/* Uses the already checked exact component, now qualified from Chicago's
 * actual adapter/body/tables. This is a logical render snapshot, not C++ ABI. */
typedef struct {miami_decimator_adapter_t adapter;original_chicago_dyn_queue queue;} original_chicago_decimator;
int original_chicago_decimator_init(original_chicago_decimator *s);
/* Kernel0frequency/1wet/2hiddenbit; native wrapper2/3/4 map minus2. */
int original_chicago_decimator_set(original_chicago_decimator *s,unsigned kernel_parameter,int32_t value);
void original_chicago_decimator_reset(original_chicago_decimator *s);
int original_chicago_decimator_bind_events(original_chicago_decimator *s,const original_chicago_drive_event *events,uint32_t count);
void original_chicago_decimator_direct_event(original_chicago_decimator *s,unsigned type);
/* Activity and tempo setters have no numeric effect in the original body.
 * Empty calls leave pending coefficients and queued events unchanged. One
 * real invocation owns local64-sample latch indexing; no chunk re-timing. */
int original_chicago_decimator_process(original_chicago_decimator *s,int16_t *left,int16_t *right,uint32_t count);
/* Event-free held-zero span; refusal leaves all state unchanged. */
int original_chicago_decimator_advance_zero(original_chicago_decimator *,uint32_t);
/* Original Bypass is an identity body, no DSP history/tempo/activity/reset.
 * Its wrapper retains all wet/type/enable state and owns canonical edit cache.
 * This returns original integer setter result for wrapper2/3; no state write. */
int32_t original_chicago_bypass_parameter(unsigned wrapper_parameter,int32_t value);
int original_chicago_bypass_process(int16_t *left,int16_t *right,uint32_t count);
#endif
