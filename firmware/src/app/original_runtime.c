/* SPDX-License-Identifier: GPL-3.0-only */
#include "original_runtime.h"
#include <string.h>

static int runtime_result(original_runtime_part *p, int ok)
{
    if (!ok) p->status = ORIGINAL_RUNTIME_FAULT;
    return p->status;
}

int original_runtime_init(original_runtime_part *p, const uint8_t cell[32], float bpm,
    original_chicago_tuning *tuning, const original_chicago_fx2_cold_owner *fx)
{
    if (!p || !original_sound_valid(cell)) return ORIGINAL_RUNTIME_INVALID;
    unsigned kind = cell[29];
    unsigned factory = (unsigned)original_sound_factory_source(kind, cell[30]);
    /* The old active union is intentionally retired at this explicit cold
     * boundary. A caller must inspect the returned admission result. */
    memset(p, 0, sizeof *p);
    p->kind = (uint8_t)kind;
    int rc = kind == ORIGINAL_SOUND_MIAMI ?
        (original_miami_backend_init(&p->dsp.miami, factory, bpm) ? 1 : -2) :
        original_chicago_backend_init_compact(&p->dsp.chicago, factory, bpm, 0, tuning, fx);
    p->status = (int8_t)rc;
    if (rc != ORIGINAL_RUNTIME_OK) return rc;
    uint32_t edits = original_sound_edit_mask(cell);
    /* All admitted factories have neutral bend/damper. Archived performance
     * edit bits never become inputs to a newly adopted runtime instance. */
    for (unsigned i = 0; i < original_sound_count(kind); ++i)
        if ((edits & (UINT32_C(1) << i)) && !original_sound_is_performance(kind, i)) {
        float value;
        if (!original_sound_value(cell, i, &value) ||
            original_runtime_param(p, i, value) != ORIGINAL_RUNTIME_OK)
            return runtime_result(p, 0);
    }
    /* A cold adoption can occur inside the current audio boundary. Publish a
     * prepared instance so its first queued/sequence note never precedes the
     * threshold snapshot. Further prepare calls are explicitly idempotent. */
    return original_runtime_prepare(p, bpm);
}

int original_runtime_prepare(original_runtime_part *p, float bpm)
{
    if (!p || p->status != ORIGINAL_RUNTIME_OK) return p ? p->status : 0;
    int ok;
    if (p->kind == ORIGINAL_SOUND_MIAMI) {
        ok = original_miami_backend_set_tempo(&p->dsp.miami, bpm) &&
             original_miami_backend_prepare(&p->dsp.miami);
    } else {
        ok = original_chicago_backend_tempo(&p->dsp.chicago, bpm) == ORIGINAL_CHICAGO_BACKEND_OK &&
             original_chicago_backend_prepare(&p->dsp.chicago) == ORIGINAL_CHICAGO_BACKEND_OK;
    }
    return runtime_result(p, ok);
}

int original_runtime_retire(original_runtime_part *p)
{
    if (!p) return ORIGINAL_RUNTIME_INVALID;
    if (p->kind == ORIGINAL_SOUND_CHICAGO && p->status != ORIGINAL_RUNTIME_NEEDS_FX && p->dsp.chicago.ready) {
        int rc = original_chicago_backend_retire(&p->dsp.chicago);
        if (rc != ORIGINAL_CHICAGO_BACKEND_OK) return rc;
    }
    p->status = ORIGINAL_RUNTIME_INVALID;
    return ORIGINAL_RUNTIME_OK;
}

int original_runtime_param(original_runtime_part *p, unsigned parameter, float value)
{
    if (!p || p->status != ORIGINAL_RUNTIME_OK) return p ? p->status : 0;
    return runtime_result(p, p->kind == ORIGINAL_SOUND_MIAMI ?
        original_miami_backend_set_macro(&p->dsp.miami, parameter, value) :
        original_chicago_backend_set(&p->dsp.chicago, parameter, value) == ORIGINAL_CHICAGO_BACKEND_OK);
}

static int runtime_note(original_runtime_part *p, unsigned key, unsigned velocity)
{
    return runtime_result(p, p->kind == ORIGINAL_SOUND_MIAMI ?
        (velocity ? original_miami_backend_note_on(&p->dsp.miami, key, velocity) :
                    original_miami_backend_note_off(&p->dsp.miami, key)) :
        original_chicago_backend_note(&p->dsp.chicago, key, velocity) == ORIGINAL_CHICAGO_BACKEND_OK);
}

static int key_held(const original_runtime_part *p, unsigned key)
{
    for (unsigned s = 0; s < ORIGINAL_INPUT_COUNT; ++s) if (p->keys[s][key]) return 1;
    return 0;
}

/* Same-source, same-pitch MIDI offs carry no note identity. For a cancelled
 * press followed by a retrigger, keep the first ambiguous off pending for at
 * most 16 x 256 rendered frames (about 93 ms at 44.1 kHz). Another off within
 * that window releases the live press. If it never arrives, release the live
 * press at expiry instead of leaving a stuck note. An unusually late OLD off
 * cannot be distinguished from an early NEW off: expiry favors recovery. */
#define ORIGINAL_STALE_GRACE_TICKS 16u
static int runtime_stale_tick(original_runtime_part *p, unsigned frames)
{
    if (!p->stale_timers) return ORIGINAL_RUNTIME_OK;
    unsigned elapsed = (unsigned)p->stale_tick_frames + frames;
    unsigned ticks = elapsed / 256u;
    p->stale_tick_frames = (uint16_t)(elapsed % 256u);
    if (!ticks) return ORIGINAL_RUNTIME_OK;
    for (unsigned source = 0; source < ORIGINAL_INPUT_KEYS; ++source) {
        for (unsigned key = 0; key < 128; ++key) {
            uint8_t *ttl = &p->stale_ttl[source][key];
            if (!*ttl) continue;
            if (*ttl > ticks) { *ttl -= (uint8_t)ticks; continue; }
            *ttl = 0;
            --p->stale_timers;
            p->stale[source][key] = 0;
            unsigned lane = key >> 5;
            uint32_t mask = UINT32_C(1) << (key & 31u);
            if (p->stale_pending[source][lane] & mask) {
                p->stale_pending[source][lane] &= ~mask;
                if (p->keys[source][key]) {
                    --p->keys[source][key];
                    if (!key_held(p, key) && runtime_note(p, key, 0) != ORIGINAL_RUNTIME_OK)
                        return p->status;
                }
            }
        }
    }
    return ORIGINAL_RUNTIME_OK;
}

int original_runtime_key(original_runtime_part *p, unsigned source, unsigned key, unsigned velocity)
{
    if (!p || source >= ORIGINAL_INPUT_COUNT || key > 127 || velocity > 127) return 0;
    if (p->status != ORIGINAL_RUNTIME_OK) return p->status;
    uint8_t *held = &p->keys[source][key];
    if (velocity) {
        /* Saturation is an explicit fault: never wrap a count and lose an off. */
        if (*held == UINT8_MAX) return runtime_result(p, 0);
        if (source < ORIGINAL_INPUT_KEYS && p->stale[source][key] && !p->stale_ttl[source][key]) {
            /* Start the window on the fresh press, never on the REC cancel. */
            if (!p->stale_timers) p->stale_tick_frames = 0;
            p->stale_ttl[source][key] = ORIGINAL_STALE_GRACE_TICKS;
            ++p->stale_timers;
        }
        ++*held;
        return runtime_note(p, key, velocity);
    }
    if (source < ORIGINAL_INPUT_KEYS && p->stale[source][key]) {
        --p->stale[source][key];
        if (*held) p->stale_pending[source][key >> 5] |= UINT32_C(1) << (key & 31u);
        return ORIGINAL_RUNTIME_OK;
    }
    if (!*held) return ORIGINAL_RUNTIME_OK;
    --*held;
    if (source < ORIGINAL_INPUT_KEYS)
        p->stale_pending[source][key >> 5] &= ~(UINT32_C(1) << (key & 31u));
    return key_held(p, key) ? ORIGINAL_RUNTIME_OK : runtime_note(p, key, 0);
}

int original_runtime_release_source(original_runtime_part *p, unsigned source)
{
    if (!p || source >= ORIGINAL_INPUT_COUNT) return 0;
    if (p->status != ORIGINAL_RUNTIME_OK) return p->status;
    if (source < ORIGINAL_INPUT_KEYS) {
        for (unsigned key = 0; key < 128; ++key)
            if (p->stale_ttl[source][key]) --p->stale_timers;
        memset(p->stale[source], 0, sizeof p->stale[source]);
        memset(p->stale_ttl[source], 0, sizeof p->stale_ttl[source]);
        memset(p->stale_pending[source], 0, sizeof p->stale_pending[source]);
    }
    for (unsigned key = 0; key < 128; ++key) if (p->keys[source][key]) {
        p->keys[source][key] = 0;
        if (!key_held(p, key) && runtime_note(p, key, 0) != ORIGINAL_RUNTIME_OK) return p->status;
    }
    return ORIGINAL_RUNTIME_OK;
}

/* End the voice of every key this external source holds. Its pending offs
 * are kept as stale counts, so the off of a cut press cannot end a newer
 * press of the same pitch. */
int original_runtime_cut_source(original_runtime_part *p, unsigned source)
{
    if (!p || source >= ORIGINAL_INPUT_COUNT) return 0;
    if (p->status != ORIGINAL_RUNTIME_OK) return p->status;
    for (unsigned key = 0; key < 128; ++key) if (p->keys[source][key]) {
        if (source < ORIGINAL_INPUT_KEYS) {
            unsigned owed = (unsigned)p->stale[source][key] + p->keys[source][key];
            p->stale[source][key] = (uint8_t)(owed > UINT8_MAX ? UINT8_MAX : owed);
            if (p->stale_ttl[source][key]) --p->stale_timers;
            p->stale_ttl[source][key] = 0;
            p->stale_pending[source][key >> 5] &= ~(UINT32_C(1) << (key & 31u));
        }
        p->keys[source][key] = 0;
        if (!key_held(p, key) && runtime_note(p, key, 0) != ORIGINAL_RUNTIME_OK) return p->status;
    }
    return ORIGINAL_RUNTIME_OK;
}

int original_runtime_all_off(original_runtime_part *p)
{
    if (!p || p->status != ORIGINAL_RUNTIME_OK) return p ? p->status : 0;
    int ok;
    if (p->kind == ORIGINAL_SOUND_MIAMI) {
        ok = original_miami_backend_all_off(&p->dsp.miami);
    } else {
        ok = original_chicago_backend_all_off(&p->dsp.chicago) == ORIGINAL_CHICAGO_BACKEND_OK;
    }
    memset(p->keys, 0, sizeof p->keys);
    memset(p->stale, 0, sizeof p->stale);
    memset(p->stale_ttl, 0, sizeof p->stale_ttl);
    memset(p->stale_pending, 0, sizeof p->stale_pending);
    p->stale_tick_frames = p->stale_timers = 0;
    return runtime_result(p, ok);
}

int original_runtime_owned(const original_runtime_part *p)
{
    if (!p || p->status != ORIGINAL_RUNTIME_OK) return 0;
    return p->kind == ORIGINAL_SOUND_MIAMI ? original_miami_backend_voice_active(&p->dsp.miami) :
        original_chicago_backend_active(&p->dsp.chicago);
}

int original_runtime_filter_view(const original_runtime_part *p, float *eg, float *depth)
{
    if (!p || p->status != ORIGINAL_RUNTIME_OK || p->kind != ORIGINAL_SOUND_CHICAGO) return 0;
    return original_chicago_backend_filter_view(&p->dsp.chicago, eg, depth);
}

int original_runtime_voice_view(const original_runtime_part *p, float *level, float *modulation,
    unsigned *state, unsigned *key_down)
{
    if (!p || p->status != ORIGINAL_RUNTIME_OK || !level || !modulation || !state || !key_down) return 0;
    *level=0; *modulation=0; *state=6; *key_down=0;
    if (p->kind == ORIGINAL_SOUND_CHICAGO) {
        const original_chicago_voice *v=&p->dsp.chicago.voice;
        if (v->active) { *level=v->vca.level; *modulation=v->mg[0].unipolar;
            *state=v->vca.state; *key_down=v->key_down; }
    } else if (p->kind == ORIGINAL_SOUND_MIAMI) {
        int selected=-1;
        for (unsigned i=0;i<ORIGINAL_MIAMI_BACKEND_VOICES;i++) {
            const original_miami_selected_voice *v=&p->dsp.miami.voices[i];
            if (v->active && (selected<0 || v->vca.level>*level)) {
                selected=(int)i; *level=v->vca.level; *modulation=v->mg[0].unipolar;
                *state=v->vca.state; *key_down=v->key_down;
            }
        }
    } else return 0;
    return 1;
}

int original_runtime_adsr_view(const original_runtime_part *p, float values[4], unsigned *gate)
{
    if (!p || p->status != ORIGINAL_RUNTIME_OK || p->kind != ORIGINAL_SOUND_CHICAGO || !values || !gate) return 0;
    for (unsigned i=0;i<4;i++) values[i]=p->dsp.chicago.controls.vca[i];
    *gate=p->dsp.chicago.controls.routing.vca_eg_mode;
    return 1;
}

int original_runtime_motion_view(const original_runtime_part *p, float *phase, float *bipolar,
    float *current, float *target, int *active)
{
    if (!p || p->status != ORIGINAL_RUNTIME_OK) return 0;
    if (p->kind == ORIGINAL_SOUND_MIAMI)
        return original_miami_backend_motion_view(&p->dsp.miami, phase, bipolar, current, target, active);
    *phase = *bipolar = 0.0f;
    return original_chicago_backend_pitch_view(&p->dsp.chicago, current, target, active);
}

int original_runtime_env_view(const original_runtime_part *p, float adsr[4], float *level, int *state)
{
    if (!p || p->status != ORIGINAL_RUNTIME_OK || p->kind != ORIGINAL_SOUND_CHICAGO) return 0;
    return original_chicago_backend_env_view(&p->dsp.chicago, adsr, level, state);
}

int original_runtime_active(const original_runtime_part *p)
{
    if (!p || p->status != ORIGINAL_RUNTIME_OK) return 0;
    return p->kind == ORIGINAL_SOUND_MIAMI ? original_miami_backend_active(&p->dsp.miami) :
        original_chicago_backend_active(&p->dsp.chicago);
}

int original_runtime_render(original_runtime_part *p, original_runtime_workspace *w,
    float *left, float *right, unsigned frames)
{
    if (!p || !w || !left || !right || frames > 256) return 0;
    if (p->status != ORIGINAL_RUNTIME_OK) return p->status;
    if (runtime_stale_tick(p, frames) != ORIGINAL_RUNTIME_OK) return p->status;
    int ok;
    if (p->kind == ORIGINAL_SOUND_MIAMI) {
        original_miami_backend_workspace workspace = {w->miami.left, w->miami.right,
            {w->miami.dry[0], w->miami.dry[1], w->miami.scratch[0], w->miami.scratch[1], 256}};
        ok = original_miami_backend_render_stereo(&p->dsp.miami, &workspace, left, right, frames);
    } else {
        original_chicago_backend_workspace workspace = {w->chicago.left, w->chicago.right,
            w->chicago.converted[0], w->chicago.converted[1],
            {w->chicago.scratch[0], w->chicago.scratch[1], w->chicago.scratch[2], w->chicago.scratch[3]}, 256};
        ok = original_chicago_backend_render(&p->dsp.chicago, &workspace, NULL, 0,
            left, right, frames) == ORIGINAL_CHICAGO_BACKEND_OK;
    }
    return runtime_result(p, ok);
}
