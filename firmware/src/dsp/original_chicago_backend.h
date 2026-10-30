/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef X0X_ORIGINAL_CHICAGO_BACKEND_H
#define X0X_ORIGINAL_CHICAGO_BACKEND_H
#include "chicago_note_context.h"
#include "engine_events.h"

enum {
    ORIGINAL_CHICAGO_BACKEND_CONTROLS = 22,
    ORIGINAL_CHICAGO_BACKEND_FACTORIES = 81,
    ORIGINAL_CHICAGO_BACKEND_MAX_BLOCK = 4096
};
/* Test for == OK, not for a nonzero result. Rejected arguments do not poison
 * a working backend. A downstream failure is sticky until an explicit reset. */
enum {
    ORIGINAL_CHICAGO_BACKEND_OK = 1,
    ORIGINAL_CHICAGO_BACKEND_INVALID = 0,
    ORIGINAL_CHICAGO_BACKEND_NEEDS_FX_OWNER = -1,
    ORIGINAL_CHICAGO_BACKEND_FAULT = -2
};

/* Serial, caller-owned storage. All eight buffers are disjoint and remain
 * alive through end(). Drive and FX2 reuse scratch at separate stage calls.
 * No history, audio array or event queue is allocated by the backend. */
typedef struct {
    uint32_t *core_left, *core_right;
    int16_t *converted_left, *converted_right;
    int16_t *scratch[4];
    uint32_t capacity;
} original_chicago_backend_workspace;

/* The public81/22 original profile fixes native88/89 to one voice/unison1.
 * It still renders four oscillators, both channels and all128 physical keys.
 * This is not a generic native-context or arbitrary restored-state adapter.
 * Keep this object at a stable address: its descriptors point into it. */
typedef struct {
    original_chicago_program program;
    original_chicago_voice_controls controls;
    original_chicago_voice_tables tables;
    original_chicago_voice voice;
    original_pending_note pending;
    original_chicago_extra_sources extra;
    chicago_arp_control keys;
    original_held_record held[ORIGINAL_ALLOCATOR_KEYS];
    original_voice_record record;
    original_chicago_allocator allocator;
    original_chicago_global_mg global;
    original_chicago_global_mg_input global_input;
    chicago_arp_clock clock1, clock2;
    original_chicago_selected_global_stage global_stage;
    original_chicago_voice_owner voice_owner;
    original_chicago_note_context note_context;
    original_chicago_selector selector;
    chicago_arp_clock_input clock_input;
    chicago_arp_control_input control_input;
    original_chicago_scheduler_callbacks scheduler_callbacks;
    original_chicago_scheduler scheduler;
    original_chicago_engine_output output;
    original_chicago_control_owner control_owner;
    original_engine_events events;
    original_chicago_backend_workspace *workspace;
    uint32_t ready, block_frames;
    unsigned factory, fz;
    int status, block_open, block_prepared;
} original_chicago_backend;

const char *original_chicago_backend_factory_name(unsigned factory);
/* Explicit firmware cold policy: zero oscillator phases, zero histories and
 * ramps, MG phases-1, selector/noise seeds0x6d70, last-note60, cleared keys.
 * Existing original init/list-reset procedures create the owned DSP state.
 * This does not reproduce the plugin's hidden restore/warmup history.
 *
 * tuning is an already bound Equal/tonic0 process-lifetime owner. Its RNG is
 * borrowed unchanged, including across reset/factory adoption. FX resources
 * must already exist and outlive the backend. reset_all MUST implement the
 * full original reset/shared-arena ownership, and find must admit all12 public
 * types. The dedicated-history bank's public reset projection does not meet
 * this contract. No built-in substitute, bypass or history allocation exists.
 * NULL/incomplete FX owner returns NEEDS_FX_OWNER before changing any state.
 * fz selects the already exposed original conversion flush mode0/1.
 * Compile without fast-math or contraction; target uses ORIGINAL_USE_SOFT_FMA. */
int original_chicago_backend_init(original_chicago_backend *, unsigned factory,
    float bpm, unsigned fz, original_chicago_tuning *, const original_chicago_fx2_owner *);
/* Owner-approved compact firmware policy: original types 0/4/5/10/18/22/24,
 * selected-state storage, and fresh selection with the old type/tail discarded.
 * This is a distinct contract, not the original all30/shared-arena owner.
 * factory stays the raw original 0..80 ID. Unsupported factory/type requests
 * return INVALID before mutation; no omitted algorithm becomes Bypass.
 * The cold owner is initialized beforehand, remains at a stable disjoint
 * address, and outlives this backend. Init/reset discards its prior selection
 * and queue exactly once; the first output constructs the factory's real FX.
 * All voice, edit, tempo and engine event semantics match the full entry point. */
int original_chicago_backend_init_compact(original_chicago_backend *, unsigned factory,
    float bpm, unsigned fz, original_chicago_tuning *, const original_chicago_fx2_cold_owner *);
/* A new factory adoption resets this whole firmware instance and its FX owner.
 * It neither reads old banks nor decodes an old bass303 project. A healthy
 * open block must finish first; a failed block is explicitly discarded. */
int original_chicago_backend_reset(original_chicago_backend *, unsigned factory);
/* Explicit cold disposal before its storage becomes another engine. Retires
 * active/old external kernels and releases borrowed descriptors. Histories
 * become inactive; the next init resets its admitted owner exactly once.
 * A healthy in-flight block cannot be disposed. init recovers it. */
int original_chicago_backend_retire(original_chicago_backend *);
int original_chicago_backend_set(original_chicago_backend *, unsigned macro, float normalized);
int original_chicago_backend_tempo(original_chicago_backend *, float bpm);
/* Idempotent preparation before serial frame-zero input. The following begin
 * consumes the prepared snapshot instead of preparing after the input. */
int original_chicago_backend_prepare(original_chicago_backend *);
/* Key events occur after prepare() (before begin, at frame zero), or after
 * begin() and before the corresponding frame(). Each
 * released key is sent with velocity0; this is not a single global gate-off.
 * Direct calls precede queued events at the next frame. Put a mixed sequence
 * in the immutable queue when it needs another equal-frame order. */
int original_chicago_backend_note(original_chicago_backend *, unsigned key, unsigned velocity);
int original_chicago_backend_release(original_chicago_backend *, unsigned key);
int original_chicago_backend_damper(original_chicago_backend *, unsigned value);
int original_chicago_backend_all_off(original_chicago_backend *);
/* Reports key/voice activity only. False is NOT permission to skip FX tails.
 * An initialized backend continues rendering until its owner has a separate
 * exact output/history idle proof or performs an explicit cold reset. */
int original_chicago_backend_active(const original_chicago_backend *);
/* UI only: the live filter EG level (0..1) and the compiled env depth. Plain
 * float reads of audio-owned state; a torn frame is a one-frame glyph glitch. */
int original_chicago_backend_filter_view(const original_chicago_backend *, float *eg, float *depth);

/* Validate the complete immutable queue and workspace before block prepare.
 * Queue events run before each frame's gate/MG/control/audio. begin() prepares
 * from the preceding control snapshot, so frame0 edits retain native deferral.
 * FX event queues are distinct caller-owned resources, never aliases of this
 * queue; the admitted FX owner must bind them at its actual invocation point.
 * Begin/frame/end preserve one full output-wrapper invocation per block.
 * Caller may inspect core_left/core_right before end for pre-FX evidence;
 * those words are not finished instrument audio. End replaces them in place
 * with the exact Drive/FX2/master result, then copies to disjoint float L/R.
 * No block-sized temporary or silent error fill is used. */
int original_chicago_backend_begin(original_chicago_backend *,
    original_chicago_backend_workspace *, const original_engine_event *,
    uint32_t event_count, uint32_t frames);
int original_chicago_backend_frame(original_chicago_backend *);
int original_chicago_backend_end(original_chicago_backend *, float *left, float *right);
int original_chicago_backend_render(original_chicago_backend *,
    original_chicago_backend_workspace *, const original_engine_event *,
    uint32_t event_count, float *left, float *right, uint32_t frames);
#endif
