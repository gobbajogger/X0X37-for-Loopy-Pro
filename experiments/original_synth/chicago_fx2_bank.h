/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_FX2_BANK_H
#define ORIGINAL_CHICAGO_FX2_BANK_H
#include "chicago_fx2_wrapper.h"
#include "chicago_dynamics.h"
#include "chicago_smallfx.h"
#include "chicago_modulation.h"
#include "chicago_fx_history.h"
typedef struct original_chicago_fx2_bank original_chicago_fx2_bank;
typedef struct {original_chicago_fx2_kernel kernel;original_chicago_fx2_bank *bank;uint8_t type;} original_chicago_fx2_binding;
typedef struct {int16_t *data;size_t slots;} original_chicago_fx2_i16_storage;
typedef struct {
 original_chicago_fx2_i16_storage short_delay,bpm_delay,gate,hall,chorus;
 float *ensemble_left,*ensemble_right;
 uint32_t ensemble_slots; /* 1024 compact or4096 dense, each channel. */
 chfx_hooks hall_context;
} original_chicago_fx2_storage;
struct original_chicago_fx2_bank {
 original_chicago_ring ring;
 original_chicago_chorus chorus;
 original_chicago_phaser phaser;
 original_chicago_ensemble ensemble;
 original_chicago_mastercomp mastercomp;
 original_chicago_peaking peaking;
 original_chicago_decimator decimator;
 chfx_short short_delay;chfx_bpm bpm_delay;chfx_gate gate;chfx_hall hall;
 original_chicago_drive_queue history_queue[4];
 int32_t history_event_origin;
 chfx_hooks hall_context;
 original_chicago_fx2_binding binding[12];
};
/* All12 actual selected public algorithms, persistent scalars and supplied
 * histories. This allocation-free constructor requires disjoint storage for
 * each live history. These bytes do NOT fit the current device pool: this is
 * a checked host/reference composition and an explicit target-admission seam,
 * not a new firmware pool or a justified alternative to the rejected backend.
 * Bind no occupied stock/sample/send buffers. No history on a stack frame.
 * 681372 history bytes per bank with compact Ensemble, plus states/scratch.
 * Failed size admission leaves the object and all supplied bytes untouched.
 * Hall's original BasicEffect context hook is explicit, never guessed. */
int original_chicago_fx2_bank_init(original_chicago_fx2_bank *,const original_chicago_fx2_storage *);
const original_chicago_fx2_kernel *original_chicago_fx2_bank_find(void *,unsigned original_type);
int original_chicago_fx2_bank_events(original_chicago_fx2_bank *,const original_chicago_drive_event *,uint32_t count,int32_t signed_frame_origin);
void original_chicago_fx2_bank_direct_event(original_chicago_fx2_bank *,unsigned type);
/* Public projection ONLY: persistent public objects in original type order,
 * Hall begin/reset/retire; all other public resets. This is not the original
 * all30 reset/shared-buffer alias proof. For an original wrapper reset, the
 * owner must provide that exact callback to wrapper_init. */
void original_chicago_fx2_bank_reset_public_projection(void *);
#endif
