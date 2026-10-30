/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_SELECTOR_H
#define ORIGINAL_CHICAGO_SELECTOR_H
#include "chicago_arp_control.h"
/* Value snapshots of19d8/dc/e0/e4/e8, not original pointer ABI. Initialization
 * comes from the exact original reset/event producer, never guessed defaults.
 * Range edits do not clamp a retained octave from the preceding range. */
typedef struct {int32_t cursor,last_key,octave,direction;uint16_t random;} original_chicago_selector;
typedef struct {int32_t duration,remaining,last_note;} original_chicago_gate;
typedef struct {uint32_t mode,range,note_mode;} original_chicago_selector_controls;
typedef struct {
 /* Original inline active-record release/mark/list-move phase. Real allocator
  * owns records and voice states, including damper/Hold and VCA!=6 handling. */
 void (*release_active)(void *);
 /* Original8cd0 handoff. Preserve generated note0..187 and raw velocity; no
  * MIDI127 clamp. Allocator owns unison/full/pending voice initialization. */
 void (*spawn)(void *,int32_t generated_note,uint32_t velocity);
} original_chicago_selector_callbacks;
/* Original8ff4 selection and gate/note-mode dispatch. Caller serially owns all
 * states, stable key arrays/counts, controls and total synchronous callbacks.
 * Native count mismatches are retained, cursor[-1,128],last_key[0,127],
 * octave[1,6],direction0/1,range[1,6],mode[0,3],gate nonnegative. None of these
 * structs overlaps another. Invalid input is rejected before state/callbacks.
 * Return-1 reports a proven original unbounded loop: nonzero count with no
 * live array entries in pureUp/Down. This is not successful silence/fallback.
 * Full admission requires an exact producer-exclusion proof or an authorized
 * separately tested correction; this component cannot activate that path.
 * Zero keys releases active records without changing current_note or gate.
 * Only note_mode1/3 release then spawn; other native modes keep that boundary.
 * This module supplies no MIDI transport, global tuning, allocator or startup. */
int original_chicago_selector_step(original_chicago_selector *,chicago_arp_control *,
 original_chicago_gate *,const original_chicago_selector_controls *,
 const original_chicago_selector_callbacks *,void *);
/* Original55e44..55e60, after queued frame events and before ARP/voice clocks.
 * A positive counter decrements; its zero edge copiescurrent->last_note only.
 * There is no invented NoteOff on this edge. */
void original_chicago_gate_tick(original_chicago_gate *,int32_t current_note);
#endif
