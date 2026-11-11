/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_SCHEDULER_H
#define ORIGINAL_CHICAGO_SCHEDULER_H
#include "chicago_allocator.h"
#include "chicago_selector.h"
typedef struct {
 const original_allocator_callbacks *voice;
 /* Actual55e64..55f38 MG1 owner, including its key-reset consumption,
  * accumulator, waveform and RNG. It runs after gate tick and before MG2.
  * State and converted inputs are caller-owned; no defaults are supplied. */
 void (*mg1_tick)(void *,original_chicago_allocator *);
 /* Actual55f84..56004 MG2 notification, before selector dispatch. The owner
  * updates real MG2 waveform/phase/RNG fields and may update clock inputs at
  * their established use points. It must not simulate this as NoteOff. */
 void (*mg2_tempo_notify)(void *,original_chicago_allocator *);
} original_chicago_scheduler_callbacks;
typedef struct {
 original_chicago_allocator *allocator;
 original_chicago_selector *selector;
 chicago_arp_clock *clock;
 const chicago_arp_clock_input *clock_input;
 chicago_arp_control_input *control_input;
 const original_chicago_scheduler_callbacks *callbacks;
 void *context;
 int dispatch_result;
} original_chicago_scheduler;
/* Borrows all inputs without resetting them or allocating resources. Keys
 * exist only at allocator->keys. Allocator owns gate, four nonrandom selector
 * fields and MG key-reset flags; selector owns its16-bit random state. Clock
 * owns accumulator/threshold; its key_reset is a synchronized scalar view.
 * All calls are serial, with total synchronous callbacks and stable storage.
 * The global/startup/controller/MG/tuning owners supply exact snapshots.
 * No cold constructor, host event queue or persistence format is inferred. */
int original_chicago_scheduler_bind(original_chicago_scheduler *,
 original_chicago_allocator *,original_chicago_selector *,chicago_arp_clock *,
 const chicago_arp_clock_input *,chicago_arp_control_input *,
 const original_chicago_scheduler_callbacks *,void *context);
/* Block preparation54d94. Input enable/latch must be0/1; raw current-note
 * lookup must lie0..255. Invalid inputs return0 before mutation. Copies the
 * explicit damper byte to allocator's same original f5 view before callbacks. */
int original_chicago_scheduler_prepare(original_chicago_scheduler *);
/* Immediate native108/internal event4. Canonical value0..127; low+Hold0
 * cleanup order is held, voices, then54d94 even for an unchanged damper byte. */
int original_chicago_scheduler_damper(original_chicago_scheduler *,uint32_t value);
/* Actual55710 type0 route. Velocity0 goes directly to NoteOff; generated
 * selector velocity0 still goes to spawn and never enters this route. */
int original_chicago_scheduler_note(original_chicago_scheduler *,int32_t key,int32_t velocity);
/* Explicit selector dispatch, also used by R3 clock callbacks. Return1 success,
 * 0 invalid contract, -1 original unbounded Up/Down count mismatch. A caller
 * must preserve/report a nonpositive result and cannot call it a silent pass.
 * Generated note/velocity are captured by selector before release callback. */
int original_chicago_scheduler_select(original_chicago_scheduler *);
/* Call after all queued frame events: gate tick -> real MG1 tick -> R3 MG2
 * key-reset/BPM/threshold/notification/selector. Then caller runs voice control
 * and audio at original57010. Outer FX/lifecycle follows at its own boundary.
 * No NoteOff is added on gate expiry. Clock inputs stay live through callbacks.
 * Compile with no fast-math and -ffp-contract=off. */
int original_chicago_scheduler_after_events(original_chicago_scheduler *);
/* Event-free, disabled-voice span. Jumps exact rounded clock additions and
 * dispatches the original callbacks on clock edges. Caller must exclude live
 * physical/latched keys and voice/restart activity for the entire span, and
 * keep BPM and both thresholds unchanged through those callbacks. */
int original_chicago_scheduler_quiet(original_chicago_scheduler *,chicago_arp_clock *mg1_clock,uint32_t frames);
/* Applies k<=frames leading after_events frames that have no clock edge and
 * no key reset (no callback, no step), the interior part of _quiet. Returns k. */
uint32_t original_chicago_scheduler_skip(original_chicago_scheduler *,chicago_arp_clock *mg1_clock,uint32_t frames);
/* Voice control action0 leaves ownership; action1 performs A818 by voice ID;
 * action2 validates and retains active ownership. Caller handles action2's
 * exact pending full/partial initializers and same-frame render. No global
 * target, MG input or pending resource is fabricated here. */
int original_chicago_scheduler_voice_action(original_chicago_scheduler *,unsigned voice_id,unsigned action);
/* Producer-tested source only. No activation, review or native-fit claim. */
#endif
