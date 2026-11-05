/* SPDX-License-Identifier: GPL-3.0-only */
#include "original_miami_backend.h"
#include "../../../experiments/original_synth/binary32_classify.h"
#include "../../../experiments/original_synth/binary32_math.h"
#include "../../../experiments/original_synth/miami_idle_clock.h"
#include "../../../experiments/original_synth/miami_zero.h"
#include "../../../experiments/original_synth/motion_tables_private.h"
#include <string.h>

#define BACKEND_READY UINT32_C(0x4d424531)

static int valid_bpm(float bpm)
{
    return original_binary32_isfinite(bpm) && bpm >= 20.0f && bpm <= 400.0f;
}

int original_miami_backend_status(const original_miami_backend *s)
{
    return s && s->ready == BACKEND_READY && s->status && s->voice_owner.status &&
        s->note_context.status && s->control_owner.status;
}

static int completed(original_miami_backend *s, int result)
{
    if (!result || !s->voice_owner.status || !s->note_context.status || !s->control_owner.status) {
        s->status = 0;
        return 0;
    }
    return 1;
}

int original_miami_backend_init(original_miami_backend *s, unsigned factory, float bpm)
{
    original_miami_program program;
    original_miami_voice_controls controls;
    if (!s || !valid_bpm(bpm) || !original_miami_program_init(&program, factory) ||
        !original_miami_voice_compile(&controls, &program)) return 0;

    memset(s, 0, sizeof *s);
    s->program = program;
    s->controls = controls;
    const original_ms20_tables filter = {
        original_miami_a44100, original_miami_b44100, original_miami_r, original_miami_cap
    };
    if (!original_miami_voice_tables_bind_immutable(&s->tables, &filter, original_pitch44100) ||
        !original_chicago_allocator_bind(&s->allocator, &s->keys, s->held, s->records,
            ORIGINAL_MIAMI_BACKEND_VOICES, ORIGINAL_MIAMI_BACKEND_VOICES,
            ORIGINAL_MIAMI_BACKEND_VOICES, 1, 0, 1)) return 0;
    s->allocator.mg1_key_sync = s->allocator.mg2_key_sync = 1;
    s->allocator.arp_range = 1;
    s->allocator.scan_index = -1;
    s->allocator.previous_index = 60;
    s->allocator.octave = 1;
    for (unsigned i = 0; i < ORIGINAL_MIAMI_BACKEND_VOICES; ++i)
        if (!original_miami_selected_voice_init(&s->voices[i], &s->tables, i, 60)) return 0;

    float rate1, rate2;
    if (!original_miami_program_native(&s->program, 44, &rate1) ||
        !original_miami_program_native(&s->program, 50, &rate2)) return 0;
    s->global_input = (original_chicago_global_mg_input){44100, 44100.0f,
        rate1, rate2, controls.mg_shape[0], controls.mg_shape[1], 1, 1};
    /* Both thresholds are prepared before any event can enter this owner. */
    s->clock1.threshold = s->clock2.threshold = 1.0f;
    original_miami_note_producers producers = original_miami_note_context_callbacks(&s->note_context);
    if (!original_miami_voice_owner_bind(&s->voice_owner, s->voices, s->pending, s->extra,
        ORIGINAL_MIAMI_BACKEND_VOICES, &s->allocator, &s->controls, &s->tables, &producers, bpm) ||
        !original_miami_note_context_bind(&s->note_context, &s->voice_owner, &s->global,
            &s->global_input, &s->clock1, &s->clock2, 0, 0, 1, 1)) return 0;
    s->selector = (original_chicago_selector){-1, 60, 1, 0, 0x6d70};
    s->clock_input = (chicago_arp_clock_input){bpm, 1};
    s->callbacks = original_miami_note_context_scheduler_callbacks();
    if (!original_chicago_scheduler_bind(&s->scheduler, &s->allocator, &s->selector,
        &s->clock2, &s->clock_input, &s->control_input, &s->callbacks, &s->voice_owner)) return 0;
    original_miami_output_init(&s->output);
    if (!original_miami_control_owner_bind(&s->control_owner, &s->program, &s->controls,
        &s->output, &s->scheduler, &s->global_input) ||
        !original_miami_note_context_core_reset(&s->note_context) ||
        !original_miami_output_apply(&s->output, &s->program, 10) ||
        !original_chicago_scheduler_damper(&s->scheduler, controls.damper ? 127u : 0u) ||
        !original_miami_control_owner_block_prepare(&s->control_owner, &s->clock1)) return 0;
    s->ready = BACKEND_READY;
    s->status = 1;
    return 1;
}

int original_miami_backend_load_factory(original_miami_backend *s, unsigned factory)
{
    original_miami_program program;
    original_miami_voice_controls controls;
    if (!original_miami_backend_status(s) || !original_miami_program_init(&program, factory) ||
        !original_miami_voice_compile(&controls, &program)) return 0;
    s->program = program;
    s->controls = controls;
    s->global_input.waveform1 = controls.mg_shape[0];
    s->global_input.waveform2 = controls.mg_shape[1];
    if (!completed(s, original_miami_output_apply(&s->output, &s->program, 10))) return 0;
    return completed(s, original_chicago_scheduler_damper(&s->scheduler, controls.damper ? 127u : 0u));
}

int original_miami_backend_set_macro(original_miami_backend *s, unsigned control, float normalized)
{
    if (!original_miami_backend_status(s) || control >= ORIGINAL_MIAMI_BACKEND_CONTROLS ||
        !original_binary32_isfinite(normalized) || normalized < 0.0f || normalized > 1.0f) return 0;
    return completed(s, original_miami_control_owner_edit(&s->control_owner, control, normalized));
}

const original_miami_program *original_miami_backend_program(const original_miami_backend *s)
{
    return original_miami_backend_status(s) ? &s->program : NULL;
}

int original_miami_backend_set_tempo(original_miami_backend *s, float bpm)
{
    if (!original_miami_backend_status(s) || !valid_bpm(bpm)) return 0;
    s->voice_owner.bpm = bpm;
    s->clock_input.bpm = bpm;
    return 1;
}

int original_miami_backend_note_on(original_miami_backend *s, unsigned key, unsigned velocity)
{
    if (!original_miami_backend_status(s) || key > 127 || !velocity || velocity > 127) return 0;
    return completed(s, original_chicago_scheduler_note(&s->scheduler, (int32_t)key, (int32_t)velocity));
}

int original_miami_backend_note_off(original_miami_backend *s, unsigned key)
{
    if (!original_miami_backend_status(s) || key > 127) return 0;
    return completed(s, original_chicago_scheduler_note(&s->scheduler, (int32_t)key, 0));
}

int original_miami_backend_damper(original_miami_backend *s, unsigned value)
{
    if (!original_miami_backend_status(s) || value > 127) return 0;
    return completed(s, original_chicago_scheduler_damper(&s->scheduler, value));
}

int original_miami_backend_all_off(original_miami_backend *s)
{
    if (!original_miami_backend_status(s)) return 0;
    original_chicago_allocator_all_notes_off(&s->allocator, original_miami_voice_owner_callbacks(), &s->voice_owner);
    return completed(s, 1);
}

int original_miami_backend_clear_keys(original_miami_backend *s)
{
    if (!original_miami_backend_status(s)) return 0;
    original_chicago_allocator_clear_keys(&s->allocator, original_miami_voice_owner_callbacks(), &s->voice_owner);
    return completed(s, 1);
}

int original_miami_backend_panic(original_miami_backend *s)
{
    if (!original_miami_backend_status(s)) return 0;
    if (!completed(s, original_miami_note_context_core_reset(&s->note_context))) return 0;
    original_miami_fx_reset(&s->output.fx);
    return 1;
}

int original_miami_backend_transport(original_miami_backend *s, unsigned running)
{
    if (!original_miami_backend_status(s) || running > 1) return 0;
    if (s->running && !running && !original_miami_backend_all_off(s)) return 0;
    s->running = running;
    return 1;
}

static int overlap(const void *a, size_t an, const void *b, size_t bn)
{
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < an : x - y < bn;
}

static int words_valid(const original_miami_backend *s, const original_miami_fx_workspace *w,
    const uint32_t *left, const uint32_t *right, uint32_t n)
{
    if (!w || n > ORIGINAL_MIAMI_BACKEND_MAX_BLOCK || n > w->capacity ||
        overlap(w, sizeof *w, s, sizeof *s)) return 0;
    const void *p[6] = {left, right, w->dryL, w->dryR, w->scratchL, w->scratchR};
    for (unsigned i = 0; i < 6; ++i) {
        size_t bytes = (size_t)n * (i < 4 ? sizeof(uint32_t) : sizeof(int16_t));
        if (!p[i] || overlap(p[i], bytes, s, sizeof *s) || overlap(p[i], bytes, w, sizeof *w)) return 0;
        for (unsigned j = 0; j < i; ++j)
            if (overlap(p[i], bytes, p[j], (size_t)n * (j < 4 ? sizeof(uint32_t) : sizeof(int16_t)))) return 0;
    }
    return 1;
}

static int events_valid(const original_engine_event *events, uint32_t count, uint32_t frames)
{
    if (count && !events) return 0;
    size_t bytes = (size_t)count * sizeof *events;
    if (count && bytes / sizeof *events != count) return 0;
    for (uint32_t i = 0; i < count; ++i) {
        const original_engine_event *e = events + i;
        if (e->frame >= frames || e->kind > ORIGINAL_EVENT_MUTE ||
            (i && e->frame < events[i - 1].frame)) return 0;
        switch (e->kind) {
        case ORIGINAL_EVENT_NOTE:
            if (e->parameter > 127 || e->value > 127) return 0;
            break;
        case ORIGINAL_EVENT_MACRO: {
            float value = original_math_float(e->value);
            if (e->parameter >= ORIGINAL_MIAMI_BACKEND_CONTROLS ||
                !original_binary32_isfinite(value) || value < 0.0f || value > 1.0f) return 0;
            break;
        }
        case ORIGINAL_EVENT_DAMPER:
            if (e->parameter || e->value > 127) return 0;
            break;
        case ORIGINAL_EVENT_MUTE:
            if (e->parameter || e->value > 255) return 0;
            break;
        default:
            if (e->parameter || e->value) return 0;
            break;
        }
    }
    return 1;
}

static int event_apply(original_miami_backend *s, const original_engine_event *e)
{
    switch (e->kind) {
    case ORIGINAL_EVENT_NOTE:
        return e->value ? original_miami_backend_note_on(s, e->parameter, e->value) :
            original_miami_backend_note_off(s, e->parameter);
    case ORIGINAL_EVENT_MACRO:
        return original_miami_backend_set_macro(s, e->parameter, original_math_float(e->value));
    case ORIGINAL_EVENT_DAMPER:
        return original_miami_backend_damper(s, e->value);
    case ORIGINAL_EVENT_ALL_OFF:
        return original_miami_backend_all_off(s);
    case ORIGINAL_EVENT_CLEAR_KEYS:
        return original_miami_backend_clear_keys(s);
    case ORIGINAL_EVENT_MUTE:
        original_master_mute(&s->output.master, (uint8_t)e->value);
        return 1;
    default:
        return 0;
    }
}

int original_miami_backend_prepare(original_miami_backend *s)
{
    if (!original_miami_backend_status(s)) return 0;
    if (s->block_prepared) return 1;
    if (!completed(s, original_miami_control_owner_block_prepare(&s->control_owner, &s->clock1))) return 0;
    s->block_prepared = 1;
    return 1;
}

/* Only the selected Miami ARP-off, empty-key graph is eligible. Inactive
 * voices freeze their oscillator/filter/envelope history in the scalar path;
 * do not reset that history or decide silence from a small amplitude. */
static int silent_block_ready(const original_miami_backend *s)
{
    if (s->keys.committed_enable || s->keys.physical_count ||
        s->allocator.held_active.count || s->allocator.voice_active.count ||
        s->allocator.gate_remaining || s->allocator.mg1_key_reset ||
        s->allocator.mg2_key_reset || s->clock1.key_reset || s->clock2.key_reset ||
        s->voice_owner.count != ORIGINAL_MIAMI_BACKEND_VOICES ||
        s->allocator.capacity != ORIGINAL_MIAMI_BACKEND_VOICES ||
        s->global.counter < 0 || s->global.counter >= 44100 ||
        s->global_input.sample_rate_integer != 44100 ||
        s->global_input.tempo_sync1 != 1 || s->global_input.tempo_sync2 != 1 ||
        s->global_input.waveform1 > 3 || s->global_input.waveform2 > 3 ||
        !original_binary32_isfinite(s->clock1.accumulator) || s->clock1.accumulator < 0 ||
        !original_binary32_isfinite(s->clock2.accumulator) || s->clock2.accumulator < 0 ||
        !original_binary32_isfinite(s->clock1.threshold) || s->clock1.threshold <= 0 ||
        !original_binary32_isfinite(s->clock2.threshold) || s->clock2.threshold <= 0)
        return 0;
    for (unsigned id = 0; id < ORIGINAL_MIAMI_BACKEND_VOICES; ++id)
        if (s->voices[id].active || s->voices[id].awaiting_restart) return 0;
    return s->output.fx.decimator.kernel.held_left == 0 &&
        s->output.fx.decimator.kernel.held_right == 0;
}

/* Miami's selected global MGs are tempo-synced shapes 0..3. At their real
 * 690 Hz ticks, multiplying a normal phase by 1/2 (or a [0,1] ratio by 2)
 * is exact, so the following addition has the same rounding as software FMA.
 * Keep the general helper for tiny phases, malformed clocks or other graphs.
 * Inactive per-voice MG/noise/ramps and the disabled ARP RNG stay frozen. */
static int advance_silent_global(original_miami_backend *s)
{
    original_chicago_global_mg_osc *mg[2] = {&s->global.mg1, &s->global.mg2};
    const chicago_arp_clock *clock[2] = {&s->clock1, &s->clock2};
    for (unsigned i = 0; i < 2; ++i) {
        uint32_t magnitude = original_math_bits(mg[i]->phase) & 0x7fffffffu;
        if ((magnitude && magnitude < 0x01000000u) || magnitude > 0x3f800000u ||
            clock[i]->accumulator >= clock[i]->threshold)
            return completed(s, s->voice_owner.producers.helper_exit(s->voice_owner.producers.context));
    }
    s->global.counter += 690 - 44100;
    for (unsigned i = 0; i < 2; ++i) {
        unsigned shape = i ? s->global_input.waveform2 : s->global_input.waveform1;
        float phase = mg[i]->phase;
        if (shape == 0) mg[i]->output = (phase >= 0 ? -phase : phase) + 1.0f;
        else if (shape == 1) mg[i]->output = (-phase * 0.5f) + 0.5f;
        else if (shape == 2) mg[i]->output = (phase * 0.5f) + 0.5f;
        else mg[i]->output = phase >= 0 ? 0 : 1;
        mg[i]->wrapped = 0;
        float ratio = clock[i]->accumulator / clock[i]->threshold;
        mg[i]->phase = (ratio * 2.0f) - 1.0f;
    }
    ++s->note_context.helper_entries;
    return 1;
}

/* Advance the tempo clocks through real notification boundaries. Empty
 * ARP-off keys leave the selector and all noise generators frozen. */
static int advance_silent_clocks(original_miami_backend *s, uint32_t frames)
{
    uint32_t remaining = frames;
    while (remaining) {
        float a = s->clock1.accumulator, b = s->clock2.accumulator;
        uint32_t na = original_miami_idle_clock_below(&a, s->voice_owner.bpm,
            s->clock1.threshold, remaining);
        uint32_t nb = original_miami_idle_clock_below(&b, s->clock_input.bpm,
            s->clock2.threshold, remaining);
        uint32_t n = na < nb ? na : nb;
        if (na != n) {
            a = s->clock1.accumulator;
            (void)original_miami_idle_clock_below(&a, s->voice_owner.bpm, s->clock1.threshold, n);
        }
        if (nb != n) {
            b = s->clock2.accumulator;
            (void)original_miami_idle_clock_below(&b, s->clock_input.bpm, s->clock2.threshold, n);
        }
        s->clock1.accumulator = a;s->clock2.accumulator = b;
        s->scheduler.dispatch_result = 1;
        remaining -= n;
        if (remaining) {
            /* Original notification/ARP order on the threshold frame.
             * Empty ARP-off keys leave selector/noise state frozen. */
            if (!completed(s, original_chicago_scheduler_after_events(&s->scheduler))) return 0;
            --remaining;
        }
    }
    return 1;
}

static int advance_silent_block(original_miami_backend *s, uint32_t frames)
{
    if (!completed(s, original_miami_voice_owner_block_valid(&s->voice_owner))) return 0;
    uint32_t ticks = ((uint32_t)s->global.counter + frames * 690u) / 44100u;
    if (ticks > 2) {
        /* Synced shapes 0..3 neither integrate phase nor consume noise. The
         * final waveform uses the previous tick's phase; two final ticks
         * overwrite every earlier phase/output. Notifications still run at
         * their original clock frames, including any phase/output resets. */
        uint32_t skip = (44100u * (ticks - 2u) - (uint32_t)s->global.counter + 689u) / 690u;
        if (!advance_silent_clocks(s, skip)) return 0;
        s->global.counter = (int32_t)(((uint32_t)s->global.counter + skip * 690u) % 44100u);
        s->note_context.helper_entries += skip;
        frames -= skip;
    }
    while (frames) {
        uint32_t span = (44100u - 1u - (uint32_t)s->global.counter) / 690u + 1u;
        if (span > frames) span = frames;
        if (!advance_silent_clocks(s, span)) return 0;
        s->global.counter += (int32_t)(span - 1) * 690;
        s->note_context.helper_entries += span - 1;
        if (s->global.counter + 690 >= 44100) {
            if (!advance_silent_global(s)) return 0;
        } else {
            s->global.counter += 690;
            ++s->note_context.helper_entries;
        }
        frames -= span;
    }
    return 1;
}


static int render_voice_frame(original_miami_backend *s, uint32_t *left, uint32_t *right)
{
    float l = 0.0f, r = 0.0f;
    for (unsigned id = 0; id < ORIGINAL_MIAMI_BACKEND_VOICES; ++id) {
        float x, y;
        if (!completed(s, original_miami_selected_voice_render_frame(&s->voices[id],
            &s->controls, &x, &y))) return 0;
        l = l + x;
        r = r + y;
    }
    *left = original_math_bits(l);
    *right = original_math_bits(r);
    return 1;
}

static int render_prevalidated_frame(original_miami_backend *s, uint32_t *left, uint32_t *right)
{
    float l = 0.0f, r = 0.0f;
    for (unsigned id = 0; id < ORIGINAL_MIAMI_BACKEND_VOICES; ++id) {
        float x, y;
        if (!completed(s, original_miami_selected_voice_render_prevalidated(&s->voices[id],
            &s->controls, &x, &y))) return 0;
        l = l + x;
        r = r + y;
    }
    *left = original_math_bits(l);
    *right = original_math_bits(r);
    return 1;
}

int original_miami_voice_render_add(original_miami_ms20 *f, original_voice_gain *vg, original_oscillators *ph,
    const original_prepared_oscillators *p, float vco_gain, float *L, float *R, uint32_t n);
static int render_span(original_miami_backend *s, uint32_t *left, uint32_t *right, uint32_t n)
{
    while (n > 64) {   /* control spans are at most 64 frames (44100 / 690) */
        if (!render_span(s, left, right, 64)) return 0;
        left += 64; right += 64; n -= 64;
    }
    static float accL[64], accR[64];
    for (uint32_t j = 0; j < n; ++j) { accL[j] = 0.0f; accR[j] = 0.0f; }
    for (unsigned id = 0; id < ORIGINAL_MIAMI_BACKEND_VOICES; ++id) {
        original_miami_selected_voice *v = &s->voices[id];
        if (v->awaiting_restart) return completed(s, 0);
        if (!v->active) continue;
        if (!completed(s, original_miami_voice_render_add(&v->filter, &v->gain, &v->pitch.oscillator,
            &v->prepared, s->controls.vco_gain, accL, accR, n))) return 0;
    }
    for (uint32_t j = 0; j < n; ++j) {
        left[j] = original_math_bits(accL[j]);
        right[j] = original_math_bits(accR[j]);
    }
    return 1;
}

/* No queued events and ARP off means scheduler clock notifications cannot
 * spawn/replace a voice inside these spans. Run the control graph at its
 * genuine 690 Hz boundaries, retaining the original scheduler before each
 * sample. A single control update for an entire 256-frame block would lose
 * roughly four envelope/modulation updates and would not be bit-exact. */
static int render_event_free_block(original_miami_backend *s,
    uint32_t *left, uint32_t *right, uint32_t frames)
{
    if (!completed(s, original_miami_voice_owner_block_valid(&s->voice_owner))) return 0;
    uint32_t offset = 0;
    while (offset < frames) {
        if (!completed(s, original_chicago_scheduler_after_events(&s->scheduler)) ||
            !completed(s, original_miami_voice_owner_control_prevalidated(&s->voice_owner))) return 0;
        uint32_t span = frames - offset;
        for (unsigned id = 0; id < s->allocator.capacity; ++id) {
            const original_miami_selected_voice *v = &s->voices[id];
            if (v->active && v->vca.state != 6) {
                uint32_t n = 1 + (44100u - 1 - v->counter) / 690u;
                if (n < span) span = n;
            }
        }
        if (s->global.counter < 0 || s->global.counter >= 44100) span = 1;
        else {
            uint32_t n = 1 + (44100u - 1 - (uint32_t)s->global.counter) / 690u;
            if (n < span) span = n;
        }
        /* Inside a span no control tick and no voice change: the per-frame
         * scheduler/counter work does not touch the rendered state, so the
         * voices render the whole span (voice-major, the same sum order). */
        /* Frames with no clock edge or key reset only move the clocks and
         * the gate (scheduler_skip); the others run the scheduler. */
        uint32_t rest = span - 1;
        unsigned skippable = s->voice_owner.bpm == s->scheduler.clock_input->bpm;
        while (rest) {
            uint32_t k = skippable ? original_chicago_scheduler_skip(&s->scheduler, &s->clock1, rest) : 0;
            if (!k) {
                if (!completed(s, original_chicago_scheduler_after_events(&s->scheduler))) return 0;
                k = 1;
            }
            for (unsigned id = 0; id < s->allocator.capacity; ++id) {
                original_miami_selected_voice *v = &s->voices[id];
                if (v->active && v->vca.state != 6) v->counter += 690u * k;
            }
            s->global.counter += (int32_t)(690u * k);
            s->note_context.helper_entries += k;
            rest -= k;
        }
        if (!render_span(s, left + offset, right + offset, span)) return 0;
        offset += span;
    }
    return 1;
}

/* Inputs and workspace have been checked by the public entry point. Return
 * 2 only when the settled-zero seam has produced exact positive-zero words. */
static int render_event_words_validated(original_miami_backend *s, original_miami_fx_workspace *w,
    uint32_t *left, uint32_t *right, uint32_t frames, const original_engine_event *events, uint32_t count)
{
    if (count) {
        size_t event_bytes = (size_t)count * sizeof *events;
        if (event_bytes / sizeof *events != count || overlap(events, event_bytes, s, sizeof *s) ||
            overlap(events, event_bytes, w, sizeof *w)) return 0;
        const void *p[6] = {left, right, w->dryL, w->dryR, w->scratchL, w->scratchR};
        for (unsigned i = 0; i < 6; ++i)
            if (overlap(events, event_bytes, p[i], (size_t)frames *
                (i < 4 ? sizeof(uint32_t) : sizeof(int16_t)))) return 0;
    }
    if (!original_miami_backend_prepare(s)) return 0;
    if (!count && silent_block_ready(s)) {
        if (!advance_silent_block(s, frames)) return 0;
        if (original_miami_output_zero(&s->output, &s->program, w, left, right, frames)) {
            s->block_prepared = 0;
            return 2;
        }
        memset(left, 0, frames * sizeof *left);
        memset(right, 0, frames * sizeof *right);
        int result = completed(s, original_miami_output_process(&s->output, &s->program, w,
            left, right, left, right, frames, 0, NULL, 0));
        s->block_prepared = 0;
        return result;
    }
    if (!count && !s->keys.committed_enable &&
        s->voice_owner.count == ORIGINAL_MIAMI_BACKEND_VOICES &&
        s->allocator.capacity == ORIGINAL_MIAMI_BACKEND_VOICES) {
        if (!render_event_free_block(s, left, right, frames)) return 0;
    } else {
        uint32_t cursor = 0;
        for (uint32_t frame = 0; frame < frames; ++frame) {
            while (cursor < count && events[cursor].frame == frame) {
                if (!completed(s, event_apply(s, events + cursor))) return 0;
                ++cursor;
            }
            if (!completed(s, original_chicago_scheduler_after_events(&s->scheduler)) ||
                !completed(s, original_miami_voice_owner_control_all(&s->voice_owner))) return 0;
            if (!render_voice_frame(s, left + frame, right + frame)) return 0;
        }
    }
    int result = completed(s, original_miami_output_process(&s->output, &s->program, w,
        left, right, left, right, frames, 0, NULL, 0));
    s->block_prepared = 0;
    return result;
}

int original_miami_backend_render_event_words(original_miami_backend *s, original_miami_fx_workspace *w,
    uint32_t *left, uint32_t *right, uint32_t frames, const original_engine_event *events, uint32_t count)
{
    if (!original_miami_backend_status(s) || !events_valid(events, count, frames)) return 0;
    if (!frames) return 1;
    if (!words_valid(s, w, left, right, frames)) return 0;
    return render_event_words_validated(s, w, left, right, frames, events, count) != 0;
}

int original_miami_backend_render_events(original_miami_backend *s, original_miami_backend_workspace *w,
    float *left, float *right, uint32_t frames, const original_engine_event *events, uint32_t count)
{
    if (!original_miami_backend_status(s) || !events_valid(events, count, frames)) return 0;
    if (!frames) return 1;
    if (!w || !left || !right || !words_valid(s, &w->fx, w->left, w->right, frames)) return 0;
    const void *p[6] = {w->left, w->right, w->fx.dryL, w->fx.dryR, w->fx.scratchL, w->fx.scratchR};
    size_t bytes = (size_t)frames * sizeof(float);
    if (overlap(left, bytes, right, bytes) || overlap(left, bytes, s, sizeof *s) ||
        overlap(right, bytes, s, sizeof *s) || overlap(left, bytes, w, sizeof *w) ||
        overlap(right, bytes, w, sizeof *w)) return 0;
    if (count && (overlap(left, bytes, events, (size_t)count * sizeof *events) ||
        overlap(right, bytes, events, (size_t)count * sizeof *events))) return 0;
    for (unsigned i = 0; i < 6; ++i) {
        size_t n = (size_t)frames * (i < 4 ? sizeof(uint32_t) : sizeof(int16_t));
        if (overlap(left, bytes, p[i], n) || overlap(right, bytes, p[i], n) ||
            overlap(p[i], n, w, sizeof *w)) return 0;
    }
    int result = render_event_words_validated(s, &w->fx, w->left, w->right, frames, events, count);
    if (!result) return 0;
    if (result == 2) {
        original_miami_zero_floats(left, frames);
        original_miami_zero_floats(right, frames);
        return 1;
    }
    memcpy(left, w->left, bytes);
    memcpy(right, w->right, bytes);
    return 1;
}

int original_miami_backend_render_words(original_miami_backend *s, original_miami_fx_workspace *w,
    uint32_t *left, uint32_t *right, uint32_t frames)
{
    return original_miami_backend_render_event_words(s, w, left, right, frames, NULL, 0);
}

int original_miami_backend_render_stereo(original_miami_backend *s, original_miami_backend_workspace *w,
    float *left, float *right, uint32_t frames)
{
    return original_miami_backend_render_events(s, w, left, right, frames, NULL, 0);
}

int original_miami_backend_motion_view(const original_miami_backend *s, float *phase, float *bipolar,
    float *current, float *target, int *active)
{
    unsigned i, k = 0;
    if (!s || s->ready != BACKEND_READY) return 0;
    for (i = 0; i < ORIGINAL_MIAMI_BACKEND_VOICES; i++) if (s->voices[i].active) { k = i; break; }
    *phase = s->voices[k].mg[0].phase; *bipolar = s->voices[k].mg[0].bipolar;
    *current = s->voices[k].pitch.current[0]; *target = s->voices[k].pitch.target[0];
    *active = s->voices[k].active != 0;
    return 1;
}

int original_miami_backend_active(const original_miami_backend *s)
{
    if (!original_miami_backend_status(s)) return 1;
    /* Raw latched keys survive ordinary releases; only an enabled latch
     * lets the selector schedule them again. */
    if (s->keys.physical_count ||
        (s->keys.committed_enable && s->keys.effective_latch && s->keys.latched_count) ||
        s->allocator.held_active.count) return 1;
    for (unsigned i = 0; i < ORIGINAL_MIAMI_BACKEND_VOICES; ++i)
        if (s->voices[i].active || s->voices[i].awaiting_restart) return 1;
    return s->output.fx.decimator.kernel.held_left != 0 || s->output.fx.decimator.kernel.held_right != 0;
}

/* Like original_miami_backend_active, but ignores the decimator hold tail:
 * true only while keys or voices still own the part. */
int original_miami_backend_voice_active(const original_miami_backend *s)
{
    if (!original_miami_backend_status(s)) return 1;
    if (s->keys.physical_count ||
        (s->keys.committed_enable && s->keys.effective_latch && s->keys.latched_count) ||
        s->allocator.held_active.count) return 1;
    for (unsigned i = 0; i < ORIGINAL_MIAMI_BACKEND_VOICES; ++i)
        if (s->voices[i].active || s->voices[i].awaiting_restart) return 1;
    return 0;
}

int original_miami_backend_idle(const original_miami_backend *s)
{
    return original_miami_backend_status(s) && !original_miami_backend_active(s);
}
