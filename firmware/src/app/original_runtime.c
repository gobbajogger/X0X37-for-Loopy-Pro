/* SPDX-License-Identifier: GPL-3.0-only */
#include "original_runtime.h"
#include "../x0x_cold.h"
#include <string.h>

int original_runtime_kind_available(int kind)
{
    return kind == ORIGINAL_SOUND_CHICAGO || kind == ORIGINAL_SOUND_MIAMI ||
        kind == ORIGINAL_SOUND_JUNO ||
        (kind == ORIGINAL_SOUND_DRONE && X0X_DRONE_RUNTIME_ENABLED);
}

static int runtime_unavailable(const original_runtime_part *p)
{
    return p && p->kind == ORIGINAL_SOUND_DRONE && !X0X_DRONE_RUNTIME_ENABLED;
}

static void runtime_clear_unavailable(original_runtime_part *p)
{
    memset(p, 0, sizeof *p);
    p->kind = ORIGINAL_SOUND_DRONE;
    p->status = ORIGINAL_RUNTIME_UNAVAILABLE;
}

static int runtime_result(original_runtime_part *p, int ok)
{
    if (!ok) p->status = ORIGINAL_RUNTIME_FAULT;
    return p->status;
}

/* Both callers validate the cell and clear DSP storage before entry. Keeping
 * this graph at its original address preserves backend self-references. */
static X0X_COLD int runtime_adopt_cell(original_runtime_part *p, const uint8_t cell[32], float bpm,
    original_chicago_tuning *tuning, const original_chicago_fx2_cold_owner *fx)
{
    unsigned kind = cell[29];
    unsigned factory = (unsigned)original_sound_factory_source(kind, cell[30]);
    p->kind = (uint8_t)kind;
    if (!original_runtime_kind_available((int)kind)) {
        p->status = ORIGINAL_RUNTIME_UNAVAILABLE;
        return p->status;
    }
    int rc;
    if (kind == ORIGINAL_SOUND_JUNO) {
        juno6_init(&p->dsp.juno.synth,44100);
        juno6_chorus_init(&p->dsp.juno.chorus,44100);
        rc=ORIGINAL_RUNTIME_OK;
    } else if (kind == ORIGINAL_SOUND_DRONE) {
#if X0X_DRONE_RUNTIME_ENABLED
        drone_string_init(&p->dsp.drone);
        rc = ORIGINAL_RUNTIME_OK;
#else
        rc = ORIGINAL_RUNTIME_UNAVAILABLE;
#endif
    } else if (kind == ORIGINAL_SOUND_MIAMI)
        rc = original_miami_backend_init(&p->dsp.miami, factory, bpm) ? 1 : -2;
    else
        rc = original_chicago_backend_init_compact(&p->dsp.chicago, factory, bpm, 0, tuning, fx);
    p->status = (int8_t)rc;
    if (rc != ORIGINAL_RUNTIME_OK) return rc;
    if (kind == ORIGINAL_SOUND_JUNO) {
        for(unsigned i=0;i<ORIGINAL_JUNO_NPARAMS-2u;i++){
            float value;
            if(!original_sound_value(cell,i,&value) || original_runtime_param(p,i,value)!=ORIGINAL_RUNTIME_OK) return runtime_result(p,0);
        }
    }
#if X0X_DRONE_RUNTIME_ENABLED
    if (kind == ORIGINAL_SOUND_DRONE) {
        /* Factory values live in the same 32-byte patch. Adopt them once at
         * the cold boundary; no table lookups in the audio render. */
        for (unsigned i = 0; i < ORIGINAL_DRONE_NPARAMS - 2u; ++i) {
            float value;
            if (!original_sound_value(cell, i, &value) ||
                !drone_string_set(&p->dsp.drone, i, value))
                return runtime_result(p, 0);
        }
    }
#endif
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

X0X_COLD int original_runtime_init(original_runtime_part *p, const uint8_t cell[32], float bpm,
    original_chicago_tuning *tuning, const original_chicago_fx2_cold_owner *fx)
{
    if (!p || !original_sound_valid(cell)) return ORIGINAL_RUNTIME_INVALID;
    /* Cold startup/panic adoption intentionally forgets all input debt. A
     * caller must retire externally owned resources before this cold reset. */
    memset(p, 0, sizeof *p);
    return runtime_adopt_cell(p, cell, bpm, tuning, fx);
}

int original_runtime_prepare(original_runtime_part *p, float bpm)
{
    if (runtime_unavailable(p)) return p->status = ORIGINAL_RUNTIME_UNAVAILABLE;
    if (!p || p->status != ORIGINAL_RUNTIME_OK) return p ? p->status : 0;
    int ok;
    if (p->kind == ORIGINAL_SOUND_JUNO || p->kind == ORIGINAL_SOUND_DRONE) {
        /* No tempo-dependent parameters: prepare is idempotent. */
        ok = 1;
    } else if (p->kind == ORIGINAL_SOUND_MIAMI) {
        ok = original_miami_backend_set_tempo(&p->dsp.miami, bpm) &&
             original_miami_backend_prepare(&p->dsp.miami);
    } else {
        ok = original_chicago_backend_tempo(&p->dsp.chicago, bpm) == ORIGINAL_CHICAGO_BACKEND_OK &&
             original_chicago_backend_prepare(&p->dsp.chicago) == ORIGINAL_CHICAGO_BACKEND_OK;
    }
    return runtime_result(p, ok);
}

static int runtime_retire_backend(original_runtime_part *p)
{
    if (p->kind == ORIGINAL_SOUND_CHICAGO && p->status != ORIGINAL_RUNTIME_NEEDS_FX && p->dsp.chicago.ready) {
        int rc = original_chicago_backend_retire(&p->dsp.chicago);
        if (rc != ORIGINAL_CHICAGO_BACKEND_OK) return rc;
    }
    return ORIGINAL_RUNTIME_OK;
}

int original_runtime_retire(original_runtime_part *p)
{
    if (!p) return ORIGINAL_RUNTIME_INVALID;
    if (runtime_unavailable(p)) {
        runtime_clear_unavailable(p);
        return ORIGINAL_RUNTIME_OK;
    }
    int rc = runtime_retire_backend(p);
    if (rc != ORIGINAL_RUNTIME_OK) return rc;
    p->status = ORIGINAL_RUNTIME_INVALID;
    return ORIGINAL_RUNTIME_OK;
}

int original_runtime_param(original_runtime_part *p, unsigned parameter, float value)
{
    if (runtime_unavailable(p)) return p->status = ORIGINAL_RUNTIME_UNAVAILABLE;
    if (!p || p->status != ORIGINAL_RUNTIME_OK) return p ? p->status : 0;
    if(p->kind==ORIGINAL_SOUND_JUNO){
        int ok=juno6_set(&p->dsp.juno.synth,parameter,value);
        if(ok) juno6_chorus_set(&p->dsp.juno.chorus,.1f+4.9f*p->dsp.juno.synth.macro[13],5.4f,.5f*p->dsp.juno.synth.macro[14]);
        return runtime_result(p,ok);
    }
#if X0X_DRONE_RUNTIME_ENABLED
    if (p->kind == ORIGINAL_SOUND_DRONE)
        return runtime_result(p, drone_string_set(&p->dsp.drone, parameter, value));
#endif
    return runtime_result(p, p->kind == ORIGINAL_SOUND_MIAMI ?
        original_miami_backend_set_macro(&p->dsp.miami, parameter, value) :
        original_chicago_backend_set(&p->dsp.chicago, parameter, value) == ORIGINAL_CHICAGO_BACKEND_OK);
}

static int runtime_note(original_runtime_part *p, unsigned key, unsigned velocity)
{
    if(p->kind==ORIGINAL_SOUND_JUNO){
        if(velocity) return runtime_result(p,juno6_note_on(&p->dsp.juno.synth,(uint8_t)key,(uint8_t)velocity));
        juno6_note_off(&p->dsp.juno.synth,(uint8_t)key);return ORIGINAL_RUNTIME_OK;
    }
#if X0X_DRONE_RUNTIME_ENABLED
    if (p->kind == ORIGINAL_SOUND_DRONE)
        return runtime_result(p, drone_string_note(&p->dsp.drone, key, velocity));
#else
    if (runtime_unavailable(p)) return p->status = ORIGINAL_RUNTIME_UNAVAILABLE;
#endif
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

/* A canceled accepted press or an unheard refused MIDI onset still owes an
 * off. This saturating ceiling and per-pitch timer reset are the existing cut
 * policy; neither pending ambiguity nor a live key is an extra hidden count. */
static void runtime_add_stale(original_runtime_part *p, unsigned source, unsigned key, unsigned count)
{
    unsigned owed = (unsigned)p->stale[source][key] + count;
    p->stale[source][key] = (uint8_t)(owed > UINT8_MAX ? UINT8_MAX : owed);
    if (p->stale_ttl[source][key]) --p->stale_timers;
    p->stale_ttl[source][key] = 0;
    p->stale_pending[source][key >> 5] &= ~(UINT32_C(1) << (key & 31u));
}

static void runtime_cut_key(original_runtime_part *p, unsigned source, unsigned key)
{
    if (source < ORIGINAL_INPUT_KEYS)
        runtime_add_stale(p, source, key, p->keys[source][key]);
    p->keys[source][key] = 0;
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
                    if (!key_held(p, key) && !runtime_unavailable(p) &&
                        runtime_note(p, key, 0) != ORIGINAL_RUNTIME_OK)
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
    if (runtime_unavailable(p)) {
        /* Refusal is not live ownership. Still account for external on/off
         * input so an unheard press's later off cannot bypass retained debt. */
        if (source < ORIGINAL_INPUT_KEYS) {
            if (velocity) runtime_add_stale(p, source, key, 1);
            else if (p->stale[source][key]) --p->stale[source][key];
        }
        return p->status = ORIGINAL_RUNTIME_UNAVAILABLE;
    }
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

/* Disconnect/channel-context teardown ends this source lifetime. Every
 * nonzero TTL contributes exactly one timer; clearing the row also makes
 * repeated teardown idempotent without changing another source's clock. */
static void runtime_clear_source_debt(original_runtime_part *p, unsigned source)
{
    if (source < ORIGINAL_INPUT_KEYS) {
        for (unsigned key = 0; key < 128; ++key)
            if (p->stale_ttl[source][key]) --p->stale_timers;
        memset(p->stale[source], 0, sizeof p->stale[source]);
        memset(p->stale_ttl[source], 0, sizeof p->stale_ttl[source]);
        memset(p->stale_pending[source], 0, sizeof p->stale_pending[source]);
    }
}

int original_runtime_release_source(original_runtime_part *p, unsigned source)
{
    if (!p || source >= ORIGINAL_INPUT_COUNT) return 0;
    if (runtime_unavailable(p)) {
        runtime_clear_source_debt(p, source);
        memset(p->keys[source], 0, sizeof p->keys[source]);
        return p->status = ORIGINAL_RUNTIME_UNAVAILABLE;
    }
    if (p->status != ORIGINAL_RUNTIME_OK) return p->status;
    runtime_clear_source_debt(p, source);
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
    if (runtime_unavailable(p)) return p->status = ORIGINAL_RUNTIME_UNAVAILABLE;
    if (p->status != ORIGINAL_RUNTIME_OK) return p->status;
    for (unsigned key = 0; key < 128; ++key) if (p->keys[source][key]) {
        runtime_cut_key(p, source, key);
        if (!key_held(p, key) && runtime_note(p, key, 0) != ORIGINAL_RUNTIME_OK) return p->status;
    }
    return ORIGINAL_RUNTIME_OK;
}

X0X_COLD int original_runtime_replace(original_runtime_part *p, const uint8_t cell[32], float bpm,
    original_chicago_tuning *tuning, const original_chicago_fx2_cold_owner *fx)
{
    if (!p || !original_sound_valid(cell)) return ORIGINAL_RUNTIME_INVALID;
    /* This is the caller's actual hard boundary, not a queued cue preview.
     * Refuse an in-flight Chicago retirement before touching any input debt. */
    int rc = runtime_retire_backend(p);
    if (rc != ORIGINAL_RUNTIME_OK) return rc;
    for (unsigned source = 0; source < ORIGINAL_INPUT_COUNT; ++source)
        for (unsigned key = 0; key < 128; ++key)
            if (p->keys[source][key]) runtime_cut_key(p, source, key);
    /* Current voices stop here. Held/debt bookkeeping is not part of the DSP
     * union, so no self-referential graph or whole part is copied or retained. */
    memset(&p->dsp, 0, sizeof p->dsp);
    return runtime_adopt_cell(p, cell, bpm, tuning, fx);
}

int original_runtime_all_off(original_runtime_part *p)
{
    if (runtime_unavailable(p)) {
        runtime_clear_unavailable(p);
        return ORIGINAL_RUNTIME_UNAVAILABLE;
    }
    if (!p || p->status != ORIGINAL_RUNTIME_OK) return p ? p->status : 0;
    int ok;
    if(p->kind==ORIGINAL_SOUND_JUNO){
        juno6_all_off(&p->dsp.juno.synth);
        memset(p->dsp.juno.chorus.delay,0,sizeof p->dsp.juno.chorus.delay);
        p->dsp.juno.chorus.write=0;ok=1;
#if X0X_DRONE_RUNTIME_ENABLED
    } else if (p->kind == ORIGINAL_SOUND_DRONE) {
        drone_string_all_off(&p->dsp.drone);
        ok = 1;
#endif
    } else if (p->kind == ORIGINAL_SOUND_MIAMI) {
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
    if (!p || p->status != ORIGINAL_RUNTIME_OK || runtime_unavailable(p)) return 0;
#if X0X_DRONE_RUNTIME_ENABLED
    if (p->kind == ORIGINAL_SOUND_DRONE) return drone_string_active(&p->dsp.drone);
#endif
    return p->kind == ORIGINAL_SOUND_JUNO ? juno6_active(&p->dsp.juno.synth) :
        p->kind == ORIGINAL_SOUND_MIAMI ? original_miami_backend_voice_active(&p->dsp.miami) :
        original_chicago_backend_active(&p->dsp.chicago);
}

int original_runtime_filter_view(const original_runtime_part *p, float *eg, float *depth)
{
    if (runtime_unavailable(p)) {
        if (eg) *eg = 0;
        if (depth) *depth = 0;
        return 0;
    }
    if (!p || p->status != ORIGINAL_RUNTIME_OK || p->kind != ORIGINAL_SOUND_CHICAGO) return 0;
    return original_chicago_backend_filter_view(&p->dsp.chicago, eg, depth);
}

X0X_COLD int original_runtime_voice_view(const original_runtime_part *p, float *level, float *modulation,
    unsigned *state, unsigned *key_down)
{
    if (!p || !level || !modulation || !state || !key_down) return 0;
    if (runtime_unavailable(p)) {
        *level = *modulation = 0; *state = 6; *key_down = 0;
        return 0;
    }
    if (p->status != ORIGINAL_RUNTIME_OK) return 0;
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
    } else if(p->kind==ORIGINAL_SOUND_JUNO){
        for(unsigned i=0;i<JUNO6_VOICES;i++){
            const juno6_voice_t *v=&p->dsp.juno.synth.voice[i];
            if(v->stage && v->envelope>*level){*level=v->envelope;*state=v->stage==1?0u:v->stage==4?3u:1u;*key_down=v->gate;}
        }
#if X0X_DRONE_RUNTIME_ENABLED
    } else if (p->kind == ORIGINAL_SOUND_DRONE) {
        for (unsigned i = 0; i < DRONE_STRING_VOICES; ++i) {
            const drone_string_voice *v = &p->dsp.drone.voice[i];
            if (v->stage && v->envelope > *level) {
                *level = v->envelope;
                *modulation = 0.0f;
                *state = v->stage == DRONE_ATTACK ? 0u :
                    v->stage == DRONE_RELEASE ? 3u : 1u;
                *key_down = v->down;
            }
        }
#endif
    } else return 0;
    return 1;
}

int original_runtime_adsr_view(const original_runtime_part *p, float values[4], unsigned *gate)
{
    if (runtime_unavailable(p)) {
        if (values) memset(values, 0, 4 * sizeof *values);
        if (gate) *gate = 0;
        return 0;
    }
    if (!p || p->status != ORIGINAL_RUNTIME_OK || p->kind != ORIGINAL_SOUND_CHICAGO || !values || !gate) return 0;
    for (unsigned i=0;i<4;i++) values[i]=p->dsp.chicago.controls.vca[i];
    *gate=p->dsp.chicago.controls.routing.vca_eg_mode;
    return 1;
}

int original_runtime_motion_view(const original_runtime_part *p, float *phase, float *bipolar,
    float *current, float *target, int *active)
{
    if (runtime_unavailable(p)) {
        if (phase) *phase = 0;
        if (bipolar) *bipolar = 0;
        if (current) *current = 0;
        if (target) *target = 0;
        if (active) *active = 0;
        return 0;
    }
    if (!p || p->status != ORIGINAL_RUNTIME_OK) return 0;
    if (p->kind == ORIGINAL_SOUND_JUNO || p->kind == ORIGINAL_SOUND_DRONE) return 0;
    if (p->kind == ORIGINAL_SOUND_MIAMI)
        return original_miami_backend_motion_view(&p->dsp.miami, phase, bipolar, current, target, active);
    *phase = *bipolar = 0.0f;
    return original_chicago_backend_pitch_view(&p->dsp.chicago, current, target, active);
}

int original_runtime_env_view(const original_runtime_part *p, float adsr[4], float *level, int *state)
{
    if (runtime_unavailable(p)) {
        if (adsr) memset(adsr, 0, 4 * sizeof *adsr);
        if (level) *level = 0;
        if (state) *state = 6;
        return 0;
    }
    if (!p || p->status != ORIGINAL_RUNTIME_OK || p->kind != ORIGINAL_SOUND_CHICAGO) return 0;
    return original_chicago_backend_env_view(&p->dsp.chicago, adsr, level, state);
}

int original_runtime_active(const original_runtime_part *p)
{
    if (!p || p->status != ORIGINAL_RUNTIME_OK || runtime_unavailable(p)) return 0;
#if X0X_DRONE_RUNTIME_ENABLED
    if (p->kind == ORIGINAL_SOUND_DRONE) return drone_string_active(&p->dsp.drone);
#endif
    return p->kind == ORIGINAL_SOUND_JUNO ? juno6_active(&p->dsp.juno.synth) :
        p->kind == ORIGINAL_SOUND_MIAMI ? original_miami_backend_active(&p->dsp.miami) :
        original_chicago_backend_active(&p->dsp.chicago);
}

int original_runtime_render(original_runtime_part *p, original_runtime_workspace *w,
    float *left, float *right, unsigned frames)
{
    if (!p || !left || !right || frames > 256) return ORIGINAL_RUNTIME_INVALID;
    if (runtime_unavailable(p)) {
        /* Silent slots still advance already-running input deadlines. This
         * never starts a grace period or calls an unavailable DSP endpoint. */
        (void)runtime_stale_tick(p, frames);
        memset(left, 0, frames * sizeof *left);
        memset(right, 0, frames * sizeof *right);
        return p->status = ORIGINAL_RUNTIME_UNAVAILABLE;
    }
    if (!w) return ORIGINAL_RUNTIME_INVALID;
    if (p->status != ORIGINAL_RUNTIME_OK) return p->status;
    if (runtime_stale_tick(p, frames) != ORIGINAL_RUNTIME_OK) return p->status;
    int ok;
    if(p->kind==ORIGINAL_SOUND_JUNO){
        juno6_render(&p->dsp.juno.synth,left,frames);
        memcpy(right,left,frames*sizeof *right);
        juno6_chorus_process(&p->dsp.juno.chorus,left,right,frames);ok=1;
#if X0X_DRONE_RUNTIME_ENABLED
    } else if (p->kind == ORIGINAL_SOUND_DRONE) {
        /* Existing stereo part buses: no new scratch or buffer latency. */
        ok = drone_string_render(&p->dsp.drone, left, right, frames);
#endif
    } else if (p->kind == ORIGINAL_SOUND_MIAMI) {
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

