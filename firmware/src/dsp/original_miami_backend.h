/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef X0X_ORIGINAL_MIAMI_BACKEND_H
#define X0X_ORIGINAL_MIAMI_BACKEND_H

#include "../../../experiments/original_synth/miami_control_owner.h"
#include "../../../experiments/original_synth/engine_events.h"

#define ORIGINAL_MIAMI_BACKEND_RATE 44100u
#define ORIGINAL_MIAMI_BACKEND_VOICES 2u
#define ORIGINAL_MIAMI_BACKEND_CONTROLS 18u
#define ORIGINAL_MIAMI_BACKEND_FACTORIES 30u
#define ORIGINAL_MIAMI_BACKEND_MAX_BLOCK 256u

/* Firmware-owned selected context: two UNISON lanes, SINGLE trigger, Hold off,
 * ARP off, Equal/tonic0, both MGs tempo/key synchronized. These are the common
 * native controls of the 30 original programs, not two polyphonic notes.
 *
 * Storage is caller owned. Init binds pointers inside this object: do not copy
 * or move it after init. One audio/event executor owns every operation. There
 * is no heap, old WBL state, legacy bank decoder or sample-store ownership. */
typedef struct {
    original_miami_program program;
    original_miami_voice_controls controls;
    original_miami_voice_tables tables;
    original_miami_output output;
    chicago_arp_control keys;
    original_held_record held[ORIGINAL_ALLOCATOR_KEYS];
    original_voice_record records[ORIGINAL_MIAMI_BACKEND_VOICES];
    original_miami_allocator allocator;
    original_miami_selected_voice voices[ORIGINAL_MIAMI_BACKEND_VOICES];
    original_miami_pending_note pending[ORIGINAL_MIAMI_BACKEND_VOICES];
    original_miami_extra_sources extra[ORIGINAL_MIAMI_BACKEND_VOICES];
    original_chicago_global_mg global;
    original_chicago_global_mg_input global_input;
    chicago_arp_clock clock1, clock2;
    original_miami_note_context note_context;
    original_miami_voice_owner voice_owner;
    original_chicago_selector selector;
    chicago_arp_clock_input clock_input;
    chicago_arp_control_input control_input;
    original_chicago_scheduler_callbacks callbacks;
    original_chicago_scheduler scheduler;
    original_miami_control_owner control_owner;
    uint32_t ready;
    int status;
    unsigned running;
    unsigned block_prepared;
} original_miami_backend;

/* Optional float-output bridge. All six arrays have fx.capacity entries and
 * distinct storage; 20 bytes/frame total. Caller may instead render directly
 * to uint32_t output buffers with render_words and provide only 12 bytes/frame
 * of FX scratch. No buffer belongs to the live backend state. */
typedef struct {
    uint32_t *left, *right;
    original_miami_fx_workspace fx;
} original_miami_backend_workspace;

/* Explicit new firmware cold start: component init, owned core reset, original
 * factory binary32 program, effect controls and damper, then block preparation.
 * Zero oscillator phases and clock residuals; reset MG seeds; no plugin host
 * restore/warmup or plugin-startup parity is implied. Invalid factory/BPM is
 * rejected before changing the destination. BPM domain is the qualified20..400.
 * A failed internal init leaves status false. Init also recovers a failed owner. */
int original_miami_backend_init(original_miami_backend *, unsigned factory, float bpm);
/* Exact factory replacement preserves current DSP history. The project owner
 * explicitly calls panic/init when a load requires cleared transient history. */
int original_miami_backend_load_factory(original_miami_backend *, unsigned factory);
int original_miami_backend_set_macro(original_miami_backend *, unsigned control, float normalized);
const original_miami_program *original_miami_backend_program(const original_miami_backend *);
int original_miami_backend_set_tempo(original_miami_backend *, float bpm);
/* Prepare one future output block before serial frame-zero events. Repeated
 * calls before render are idempotent. This lets the firmware discover the next
 * sequencer boundary after consuming input, without a second event queue or
 * moving block preparation after those events. Render consumes this marker. */
int original_miami_backend_prepare(original_miami_backend *);

/* All physical keys0..127 must reach these calls, including releases of keys
 * that are no longer the most recent key. Do not filter to a last-held note.
 * NoteOn accepts1..127. NoteOff uses the original canonical velocity0 route.
 * Call events in their original stable order before the next rendered frame. */
int original_miami_backend_note_on(original_miami_backend *, unsigned key, unsigned velocity);
int original_miami_backend_note_off(original_miami_backend *, unsigned key);
int original_miami_backend_damper(original_miami_backend *, unsigned canonical_value);
int original_miami_backend_all_off(original_miami_backend *);
int original_miami_backend_clear_keys(original_miami_backend *);
/* Known core/effect reset, preserving program, master and oscillator phases.
 * Full cold reinitialization is init. Stop transport issues all_off once on
 * its running->stopped edge; transport never gates free-play or clock render. */
int original_miami_backend_panic(original_miami_backend *);
int original_miami_backend_transport(original_miami_backend *, unsigned running);

/* Each nonempty render call is one original control/output block. Prepare
 * thresholds BEFORE frame0 events, then events -> scheduler -> voice-owner
 * control -> ascending two-lane sum each frame, then fixed FX/master once. Master
 * ramp and Decimator-local indices make these boundaries observable. Callers
 * must not silently subdivide or merge blocks. n0 changes no state. Use the
 * events API for frame-timed input. Direct calls before prepare act between
 * blocks; explicit prepare followed by direct calls places them at frame zero
 * after preparation. The subsequent render does not prepare twice. Batches are
 * immutable through the call, completely validated before preparation, and
 * retain input order for ties. There is no hidden event queue or event limit.
 *
 * Buffers, workspace descriptor and backend must be disjoint; channels must
 * not overlap. Shape/rate/precision and effect conversion are unchanged.
 * All public input errors return0 before mutation. A downstream graph failure
 * returns0 with sticky status; already advanced state is not rolled back and
 * must not be reported as successful silence. Exactly inactive event-free
 * blocks may skip voice work while preserving clocks, global MG and FX. */
int original_miami_backend_render_words(original_miami_backend *, original_miami_fx_workspace *,
    uint32_t *left, uint32_t *right, uint32_t frames);
int original_miami_backend_render_stereo(original_miami_backend *, original_miami_backend_workspace *,
    float *left, float *right, uint32_t frames);
int original_miami_backend_render_event_words(original_miami_backend *, original_miami_fx_workspace *,
    uint32_t *left, uint32_t *right, uint32_t frames, const original_engine_event *, uint32_t event_count);
int original_miami_backend_render_events(original_miami_backend *, original_miami_backend_workspace *,
    float *left, float *right, uint32_t frames, const original_engine_event *, uint32_t event_count);

/* Activity includes held/latched keys, actual release/replacement tails and
 * Decimator-held samples. An idle result permits UI/admission decisions only:
 * clocks, global MG and effect state must still receive every audio frame.
 * Failed owners are never reported idle. */
int original_miami_backend_status(const original_miami_backend *);
int original_miami_backend_active(const original_miami_backend *);
int original_miami_backend_voice_active(const original_miami_backend *);
int original_miami_backend_idle(const original_miami_backend *);

#endif
