/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "original_sound.h"
#include "../dsp/original_chicago_backend.h"
#include "../dsp/original_miami_backend.h"

enum { ORIGINAL_INPUT_USB, ORIGINAL_INPUT_BLE, ORIGINAL_INPUT_TRS,
       ORIGINAL_INPUT_KEYS, ORIGINAL_INPUT_SEQUENCE, ORIGINAL_INPUT_COUNT };
enum { ORIGINAL_RUNTIME_OK = 1, ORIGINAL_RUNTIME_INVALID = 0,
       ORIGINAL_RUNTIME_NEEDS_FX = -1, ORIGINAL_RUNTIME_FAULT = -2 };
typedef union {
    original_chicago_backend chicago;
    original_miami_backend miami;
} original_runtime_backend;
typedef struct {
    original_runtime_backend dsp;
    uint8_t keys[ORIGINAL_INPUT_COUNT][128];
    uint8_t stale[ORIGINAL_INPUT_KEYS][128]; /* offs still owed by cut external presses */
    uint8_t stale_ttl[ORIGINAL_INPUT_KEYS][128]; /* 256-frame ticks since a canceled pitch is retriggered */
    uint32_t stale_pending[ORIGINAL_INPUT_KEYS][4]; /* first ambiguous off, one bit per pitch */
    uint16_t stale_tick_frames, stale_timers;
    uint8_t kind;
    int8_t status;
} original_runtime_part;
typedef union {
    struct { uint32_t left[256], right[256]; int16_t converted[2][256], scratch[4][256]; } chicago;
    struct { uint32_t left[256], right[256]; int32_t dry[2][256]; int16_t scratch[2][256]; } miami;
} original_runtime_workspace;

/* Cold/adoption operations use the explicit seven-type Chicago selection
 * policy. Its owner remains caller-owned at a stable address. Missing resources
 * remain a visible refusal. No old engine or bank is involved. */
int original_runtime_init(original_runtime_part *, const uint8_t cell[32], float bpm,
    original_chicago_tuning *, const original_chicago_fx2_cold_owner *);
/* Call on an initialized or zero-initialized part before cold replacement;
 * externally owned Chicago resources must be retired before overwriting it. */
int original_runtime_retire(original_runtime_part *);
int original_runtime_prepare(original_runtime_part *, float bpm);
int original_runtime_param(original_runtime_part *, unsigned parameter, float value);
int original_runtime_key(original_runtime_part *, unsigned source, unsigned key, unsigned velocity);
int original_runtime_release_source(original_runtime_part *, unsigned source);
int original_runtime_cut_source(original_runtime_part *p, unsigned source);
int original_runtime_all_off(original_runtime_part *);
/* Source/voice ownership excludes residual insert output; selector fades that output separately. */
int original_runtime_owned(const original_runtime_part *);
int original_runtime_active(const original_runtime_part *);
/* Chicago only: live filter EG level and env depth for the ENV glyph. */
int original_runtime_filter_view(const original_runtime_part *, float *eg, float *depth);
/* UI observation only: loudest active native lane, ties use lowest lane ID.
 * State is original_vca.state (0 attack, 1 decay/sustain, 3 release, 6 silent).
 * Plain reads may span one audio update; no queue, locks or DSP state changes. */
int original_runtime_voice_view(const original_runtime_part *, float *level, float *modulation,
    unsigned *state, unsigned *key_down);
int original_runtime_adsr_view(const original_runtime_part *, float values[4], unsigned *gate);
/* UI motion view: Chicago or Miami. phase/bipolar are the Miami MG1 (0 for Chicago). */
int original_runtime_motion_view(const original_runtime_part *, float *phase, float *bipolar,
    float *current, float *target, int *active);
/* Chicago only: VCA EG controls, live level and state, for the ADSR graph. */
int original_runtime_env_view(const original_runtime_part *, float adsr[4], float *level, int *state);
int original_runtime_render(original_runtime_part *, original_runtime_workspace *,
    float *left, float *right, unsigned frames);
