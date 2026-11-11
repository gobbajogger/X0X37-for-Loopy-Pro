/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_FX2_WRAPPER_H
#define ORIGINAL_CHICAGO_FX2_WRAPPER_H
#include "chicago_output.h"
/* Persistent, caller-owned original algorithm objects. Every callback is
 * synchronous and total once find() admits the type. No constructor on
 * reselection: old targets, inactive histories and queue cursors survive.
 * process() receives one real invocation, including its local index origin.
 * Null old object means dry input, not an invented identity effect instance. */
typedef struct {
 void *context;
 void (*process)(void *,int16_t *,int16_t *,uint32_t);
 void (*parameter)(void *,unsigned,unsigned);
 void (*reset)(void *);
 void (*begin)(void *);
 void (*retire)(void *);
 void (*tempo)(void *,int32_t);
 void (*activity)(void *,unsigned);
} original_chicago_fx2_kernel;
typedef struct {
 void *context;
 const original_chicago_fx2_kernel *(*find)(void *,unsigned original_type);
 /* Exact original4f4d8 walk, including all owned objects and shared buffer
  * provider side effects. A twelve-public-object projection alone is not a
  * proof of the original thirty-object reset/arena alias closure. */
 void (*reset_all)(void *);
} original_chicago_fx2_owner;
/* Explicit selected-state policy, separate from the original persistent
 * all-object/shared-arena contract above. supports() is a read-only admission
 * check. select_cold() retires the prior kernel, constructs a fresh selected
 * object and binds its storage; the wrapper then calls begin/reset/edits/tempo.
 * It must be total for admitted types. discard() retires the selected object
 * exactly once and releases borrowed queue descriptors. No old object or tail
 * survives selection. Callbacks and their caller remain serial. */
typedef struct {
 void *context;
 int (*supports)(void *,unsigned original_type);
 const original_chicago_fx2_kernel *(*select_cold)(void *,unsigned original_type);
 void (*discard)(void *);
 /* Optional exact zero-input span. Zero means refusal WITHOUT mutation,
  * including the stereo result. Success advances phase/control state as
  * process(zeros,n) would and returns its constant stereo sample. A proved
  * nonzero fixed-point residue is retained, never truncated to silence. */
 int (*advance_zero)(void *,uint32_t,int16_t constant[2]);
} original_chicago_fx2_cold_owner;
typedef struct {
 original_chicago_fx2_owner owner;
 original_chicago_fx2_cold_owner cold_owner;
 const original_chicago_fx2_kernel *active,*old;
 int32_t wet_target,wet_accumulator,progress,activity_counter,tempo_times10;
 uint8_t edit1,edit2,switch_current,switch_pending,switch_dirty;
 uint8_t pending_type,type_dirty,transitioning,cold_policy;
} original_chicago_fx2_wrapper;
typedef struct {
 int16_t *new_left,*new_right,*old_left,*old_right;
 uint32_t capacity;
} original_chicago_fx2_workspace;
/* Constructor4e200 pendingType12, switch current/pending1 dirty1, typeDirty1,
 * tempo1200, null active/old, zero wet, activity-1. Public routing setters
 * follow it. This supplies no global engine startup/restore sequence. */
int original_chicago_fx2_wrapper_init(original_chicago_fx2_wrapper *,const original_chicago_fx2_owner *);
/* Cold selection starts with pending type 0 and otherwise the same original
 * wrapper ramps. Owner initialization/disposal belongs to the enclosing owner;
 * this constructor does not call discard or select. */
int original_chicago_fx2_wrapper_init_cold(original_chicago_fx2_wrapper *,const original_chicago_fx2_cold_owner *);
int original_chicago_fx2_wrapper_supports(const original_chicago_fx2_wrapper *,unsigned original_type);
int original_chicago_fx2_wrapper_set(original_chicago_fx2_wrapper *,unsigned parameter,unsigned canonical_value);
int original_chicago_fx2_wrapper_tempo(original_chicago_fx2_wrapper *,int32_t bpm_times10);
void original_chicago_fx2_wrapper_reset(original_chicago_fx2_wrapper *);
/* Caller pre-admits all histories and retains old/current through whole
 * transition chunks. Scratch/input/state/channels are disjoint; input L/R
 * are the in-place outputs. No allocation or frame-sized stack. n<=4096.
 * n0 still commits switch/type and performs reset/edit/begin/tempo/activity;
 * no kernels run while transitioning. The enclosing output skips n0 calls.
 * A->B->A before processing still selects/resets A and may process the same
 * object twice in old-then-new order. Do not deduplicate pointer equality. */
int original_chicago_fx2_wrapper_process(original_chicago_fx2_wrapper *,original_chicago_fx2_workspace *,int16_t *,int16_t *,uint32_t);
/* Positive settled invocation only; no callbacks/state change on refusal. */
int original_chicago_fx2_wrapper_advance_zero(original_chicago_fx2_wrapper *,uint32_t,
 int16_t kernel[2],int16_t mixed[2]);
/* Under the explicit cold policy a changed pending type always retires the
 * prior object before construction, clears old/active wrapper pointers, and
 * uses the original null-to-new transition. A->B->A before processing is a
 * fresh A selection. No old kernel runs during the new transition. */
#endif
