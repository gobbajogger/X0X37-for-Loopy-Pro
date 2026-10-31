/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_ENGINE_EVENTS_H
#define ORIGINAL_ENGINE_EVENTS_H
#include "miami_control_owner.h"
#include "chicago_control_owner.h"
#include "chicago_voice_owner.h"
/* Engine-owned decoded events, NOT a guessed MIDI, host or project encoding.
 * Existing transport/storage owner maps its actual bytes/timestamps explicitly.
 * Macro.value is raw normalized binary32; note.value is canonical velocity.
 * Stable equal-frame ordering is the caller's original queue order. */
enum {ORIGINAL_EVENT_NOTE,ORIGINAL_EVENT_MACRO,ORIGINAL_EVENT_DAMPER,ORIGINAL_EVENT_ALL_OFF,ORIGINAL_EVENT_CLEAR_KEYS,ORIGINAL_EVENT_MUTE};
typedef struct {uint32_t frame,kind,parameter,value;} original_engine_event;
typedef struct {
 const original_engine_event *events;
 uint32_t count,cursor,frames,next_frame;
 original_miami_control_owner *miami;
 original_chicago_control_owner *chicago;
 int status;
} original_engine_events;
/* Exactly one backend pointer. Validate the WHOLE immutable batch and its
 * canonical domains before replacing this descriptor; old state is unchanged
 * on error. No allocation, extra queue limit, sorting, dropped ties or audio
 * buffers. Events remain owned/unchanged until consumed or explicitly rebound.
 * A program edit is one serial transaction; no event or render may interleave.
 * Caller supplies a valid admitted cold/restored key/list/DSP snapshot. */
int original_engine_events_bind(original_engine_events *,const original_engine_event *,uint32_t count,uint32_t block_frames,original_miami_control_owner *,original_chicago_control_owner *);
/* Call once per frame in order BEFORE gate/MG clocks/voice controls/audio.
 * Execute all equal-frame events in input order. Downstream failures are
 * sticky/returned; prior successful events are not falsely rolled back or
 * reported as successful silence. This function adds no clock or helper entry.
 * FX queues have their own immutable lifetimes/cursors and are not borrowed
 * from a temporary version of this canonical input descriptor. */
int original_engine_events_frame(original_engine_events *,uint32_t frame);
#endif
