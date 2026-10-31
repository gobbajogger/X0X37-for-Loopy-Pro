/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_MIAMI_FX_CHAIN_H
#define ORIGINAL_MIAMI_FX_CHAIN_H
#include "fixed_fx.h"
#include "../wobble/miami_decimator_adapter.h"
typedef struct {original_fixed_fx fx1,fx2;miami_decimator_adapter_t decimator;} original_miami_fx_chain;
/* Explicit caller-owned workspace, no per-call stack arrays/heap.
 * Storage is12*capacity bytes, so a512-frame caller need not reserve2048. */
typedef struct {
 int32_t *dryL,*dryR;
 int16_t *scratchL,*scratchR;
 uint32_t capacity;
} original_miami_fx_workspace;
void original_miami_fx_init(original_miami_fx_chain *s);
void original_miami_fx_reset(original_miami_fx_chain *s);
/* Internal second-wrapper path, after fixed FX1 has saturated both dry arrays
 * to [-32767,32767]. Requires the already validated, disjoint workspace and
 * n<=capacity. Preserve scratch, wrapper state and the original integer mixer
 * rounding even though the selected effect itself is a fixed bypass. */
void original_miami_fx_bypass_process(original_fixed_fx *s,
 original_miami_fx_workspace *w,uint32_t n);
/* Native-empty convention: n0 is a successful no-op, no inner commits.
 * Fixed Decimator/Bypass selected, BOTH outer wrappers explicitly invoked.
 * Output excludes master gain/mute, outer gating and algorithm transitions.
 * RNE/gradual/masked host contract inherited from conversion helpers.
 * Arrays n entries; no overlap with s/workspace. in/out alias permitted. */
int original_miami_fx_process(original_miami_fx_chain *s,original_miami_fx_workspace *w,
 const uint32_t *inL,const uint32_t *inR,uint32_t *outL,uint32_t *outR,uint32_t n,unsigned fz);
#endif
