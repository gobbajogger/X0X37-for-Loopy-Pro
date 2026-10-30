/* SPDX-License-Identifier: GPL-3.0-only */
#include "original_chicago_backend.h"
#include "binary32_classify.h"
#include "binary32_math.h"
#include "chicago_packed.h"
#include "motion_tables_private.h"
#include <string.h>

#define BACKEND_READY UINT32_C(0x43484231)

static int total_fx(const original_chicago_fx2_owner *owner)
{
    static const uint8_t types[12] = {0,3,4,5,10,12,13,15,16,18,22,24};
    if (!owner || !owner->find || !owner->reset_all) return 0;
    for (unsigned i = 0; i < 12; ++i) {
        const original_chicago_fx2_kernel *k = owner->find(owner->context, types[i]);
        if (!k || !k->process || !k->parameter || !k->reset || !k->begin ||
            !k->retire || !k->tempo || !k->activity) return 0;
    }
    return 1;
}

static int total_cold_fx(const original_chicago_fx2_cold_owner *owner)
{
    static const uint8_t types[7] = {0,4,5,10,18,22,24};
    if (!owner || !owner->supports || !owner->select_cold || !owner->discard) return 0;
    for (unsigned i = 0; i < 7; ++i)
        if (!owner->supports(owner->context, types[i])) return 0;
    return 1;
}

static int running(const original_chicago_backend *s)
{
    return s && s->ready == BACKEND_READY && s->status == ORIGINAL_CHICAGO_BACKEND_OK;
}

static int unavailable(const original_chicago_backend *s)
{
    return s && s->ready == BACKEND_READY ? s->status : ORIGINAL_CHICAGO_BACKEND_INVALID;
}

static int fault(original_chicago_backend *s);

static int healthy(original_chicago_backend *s)
{
    if (s->voice_owner.status && s->note_context.status && s->global_stage.status &&
        s->control_owner.status) return ORIGINAL_CHICAGO_BACKEND_OK;
    return fault(s);
}

static int fault(original_chicago_backend *s)
{
    s->status = ORIGINAL_CHICAGO_BACKEND_FAULT;
    s->workspace = NULL;
    s->block_open = s->block_prepared = 0;
    s->block_frames = 0;
    memset(&s->events, 0, sizeof s->events);
    return s->status;
}

static int tempo_valid(float bpm)
{
    return original_binary32_isfinite(bpm) && bpm >= 20 && bpm <= 400;
}

/* Verify fixed context facts before choosing the public-profile geometry.
 * These controls are not targets of any of the22 public macro mappings. */
static int profile_valid(const original_chicago_program *p)
{
    static const uint8_t ids[] = {45,51,52,56,58,59,88,89};
    static const float values[] = {0,1,1,0,1.0f/3.0f,0,0,0};
    for (unsigned i = 0; i < sizeof ids; ++i) {
        float x;
        if (!original_chicago_program_native(p, ids[i], &x) || x != values[i]) return 0;
    }
    return 1;
}

const char *original_chicago_backend_factory_name(unsigned factory)
{
    return original_chicago_program_name(factory);
}

static int initialize(original_chicago_backend *s, unsigned factory,
    float bpm, unsigned fz, original_chicago_tuning *tuning,
    const original_chicago_fx2_owner *effects, const original_chicago_fx2_cold_owner *cold)
{
    if (!s || factory >= ORIGINAL_CHICAGO_BACKEND_FACTORIES || !tempo_valid(bpm) ||
        fz > 1 || !tuning || tuning->scale || tuning->tonic) return ORIGINAL_CHICAGO_BACKEND_INVALID;
    if (cold ? !total_cold_fx(cold) : !total_fx(effects))
        return ORIGINAL_CHICAGO_BACKEND_NEEDS_FX_OWNER;
    original_chicago_tuning probe = *tuning;
    float pitch;
    if (!original_chicago_tuning_target(&probe, 60, &pitch) || pitch != 60)
        return ORIGINAL_CHICAGO_BACKEND_INVALID;
    original_chicago_program program;
    original_chicago_routing routing;
    if (!original_chicago_program_init(&program, factory) || !profile_valid(&program) ||
        !original_chicago_program_routing(&program, &routing) ||
        (cold && !cold->supports(cold->context, routing.fx[1][0])))
        return ORIGINAL_CHICAGO_BACKEND_INVALID;

    /* Firmware cold init is destructive only after argument/resource admission.
     * No initialized self-referential graph is copied to another address. */
    original_chicago_fx2_owner fx = {0};
    original_chicago_fx2_cold_owner compact = {0};
    if (cold) compact = *cold;
    else fx = *effects;
    memset(s, 0, sizeof *s);
    s->status = ORIGINAL_CHICAGO_BACKEND_FAULT;
    s->factory = factory;
    s->fz = fz;
    s->program = program;
    if (!original_chicago_voice_compile(&s->controls, &s->program) ||
        !original_chicago_voice_tables_bind(&s->tables, original_chicago_packed_lookup,
            NULL, original_pitch44100)) return s->status;

    original_chicago_voice_seed seed;
    memset(&seed, 0, sizeof seed);
    seed.last_pitch = 60;
    seed.noise = UINT32_C(0x6d70);
    for (unsigned i = 0; i < 2; ++i) {
        seed.mg[i].phase = -1;
        seed.mg[i].noise = UINT32_C(0x6d70);
    }
    if (!original_chicago_voice_init(&s->voice, &s->tables, &seed) ||
        !original_chicago_allocator_bind(&s->allocator, &s->keys, s->held, &s->record,
            1, 1, 1, 1, 0, 0)) return s->status;
    s->allocator.scan_index = -1;
    s->allocator.previous_index = 60;
    s->allocator.octave = 1;
    s->allocator.mg1_key_sync = 0;
    s->allocator.mg2_key_sync = 1;
    s->allocator.arp_mode = s->controls.routing.arp_mode;
    s->allocator.arp_range = s->controls.routing.arp_range;
    s->global.mg1.phase = s->global.mg2.phase = -1;
    s->global.mg1.random = s->global.mg2.random = UINT32_C(0x6d70);
    s->global_input = (original_chicago_global_mg_input){44100,44100,
        s->controls.mg_frequency[0],s->controls.mg_frequency[1],
        s->controls.mg_shape[0],s->controls.mg_shape[1],
        s->controls.mg_tempo[0],s->controls.mg_tempo[1]};
    chicago_arp_clock_prepare(&s->clock1, 44100, s->controls.mg_base[0], s->controls.mg_times[0]);
    chicago_arp_clock_prepare(&s->clock2, 44100,
        s->controls.routing.arp_base_note, s->controls.routing.arp_times);
    if (!original_chicago_selected_global_bind(&s->global_stage, &s->global,
        &s->global_input, &s->clock1, &s->clock2, &s->allocator)) return s->status;
    original_chicago_note_producers producers = original_chicago_note_context_callbacks(&s->note_context);
    if (!original_chicago_voice_owner_bind(&s->voice_owner, &s->voice, &s->pending,
            1, &s->allocator, &s->controls, &s->tables, &producers, bpm) ||
        !original_chicago_note_context_bind(&s->note_context, &s->voice_owner, tuning,
            &s->global_stage, &s->extra, 0, 1)) return s->status;
    s->selector = (original_chicago_selector){-1,60,1,0,0x6d70};
    s->clock_input = (chicago_arp_clock_input){bpm,s->controls.routing.arp_tempo_sync};
    s->control_input = (chicago_arp_control_input){s->controls.routing.arp_enable,
        s->controls.routing.arp_latch,s->controls.routing.damper < 64 ? 0 : 127};
    s->scheduler_callbacks = original_chicago_note_context_scheduler_callbacks();
    if (!original_chicago_scheduler_bind(&s->scheduler, &s->allocator, &s->selector,
        &s->clock2, &s->clock_input, &s->control_input, &s->scheduler_callbacks,
        &s->voice_owner)) return s->status;

    if (cold) {
        compact.discard(compact.context);
        if (!original_chicago_engine_output_init_cold(&s->output, &s->controls.routing, &compact)) return s->status;
    } else {
        fx.reset_all(fx.context);
        if (!original_chicago_engine_output_init(&s->output, &s->controls.routing, &fx)) return s->status;
    }
    if (!original_chicago_fx2_wrapper_tempo(&s->output.fx2, (int32_t)(bpm * 10.0f)) ||
        !original_chicago_control_owner_bind(&s->control_owner, &s->program,
            &s->controls, &s->output, &s->scheduler, &s->global_input)) return s->status;
    original_chicago_output_mute(&s->output.output, 0);
    s->ready = BACKEND_READY;
    s->status = ORIGINAL_CHICAGO_BACKEND_OK;
    return s->status;
}

int original_chicago_backend_init(original_chicago_backend *s, unsigned factory,
    float bpm, unsigned fz, original_chicago_tuning *tuning,
    const original_chicago_fx2_owner *effects)
{
    return initialize(s, factory, bpm, fz, tuning, effects, NULL);
}

int original_chicago_backend_init_compact(original_chicago_backend *s, unsigned factory,
    float bpm, unsigned fz, original_chicago_tuning *tuning,
    const original_chicago_fx2_cold_owner *effects)
{
    if (!effects) return ORIGINAL_CHICAGO_BACKEND_NEEDS_FX_OWNER;
    return initialize(s, factory, bpm, fz, tuning, NULL, effects);
}

int original_chicago_backend_reset(original_chicago_backend *s, unsigned factory)
{
    if (!s || s->ready != BACKEND_READY || (s->block_open && s->status == ORIGINAL_CHICAGO_BACKEND_OK))
        return ORIGINAL_CHICAGO_BACKEND_INVALID;
    if (s->output.fx2.cold_policy)
        return original_chicago_backend_init_compact(s, factory, s->voice_owner.bpm,
            s->fz, s->note_context.tuning, &s->output.fx2.cold_owner);
    return original_chicago_backend_init(s, factory, s->voice_owner.bpm,
        s->fz, s->note_context.tuning, &s->output.fx2.owner);
}

int original_chicago_backend_retire(original_chicago_backend *s)
{
    if (!s || s->ready != BACKEND_READY ||
        (s->block_open && s->status == ORIGINAL_CHICAGO_BACKEND_OK)) return ORIGINAL_CHICAGO_BACKEND_INVALID;
    original_chicago_fx2_wrapper *fx = &s->output.fx2;
    if (fx->cold_policy) fx->cold_owner.discard(fx->cold_owner.context);
    else {
        if (fx->active) fx->active->retire(fx->active->context);
        if (fx->old && fx->old != fx->active) fx->old->retire(fx->old->context);
    }
    fx->active = fx->old = NULL;
    s->workspace = NULL;
    s->block_open = s->block_prepared = 0;
    s->block_frames = s->ready = 0;
    memset(&s->events, 0, sizeof s->events);
    s->status = ORIGINAL_CHICAGO_BACKEND_INVALID;
    return ORIGINAL_CHICAGO_BACKEND_OK;
}

int original_chicago_backend_set(original_chicago_backend *s, unsigned macro, float normalized)
{
    if (!running(s)) return unavailable(s);
    if (s->block_open && s->events.next_frame >= s->block_frames) return ORIGINAL_CHICAGO_BACKEND_INVALID;
    if (!original_chicago_control_owner_edit(&s->control_owner, macro, normalized))
        return s->control_owner.status ? ORIGINAL_CHICAGO_BACKEND_INVALID : fault(s);
    return healthy(s);
}

int original_chicago_backend_tempo(original_chicago_backend *s, float bpm)
{
    if (!running(s)) return unavailable(s);
    if (s->block_open || !tempo_valid(bpm)) return ORIGINAL_CHICAGO_BACKEND_INVALID;
    if (!original_chicago_fx2_wrapper_tempo(&s->output.fx2, (int32_t)(bpm * 10.0f))) return fault(s);
    s->voice_owner.bpm = s->clock_input.bpm = bpm;
    return ORIGINAL_CHICAGO_BACKEND_OK;
}

int original_chicago_backend_note(original_chicago_backend *s, unsigned key, unsigned velocity)
{
    if (!running(s)) return unavailable(s);
    if ((!s->block_open && !s->block_prepared) ||
        (s->block_open && s->events.next_frame >= s->block_frames) ||
        key >= 128 || velocity >= 128) return ORIGINAL_CHICAGO_BACKEND_INVALID;
    if (!original_chicago_scheduler_note(&s->scheduler, (int32_t)key, (int32_t)velocity)) return fault(s);
    return healthy(s);
}

int original_chicago_backend_release(original_chicago_backend *s, unsigned key)
{
    return original_chicago_backend_note(s, key, 0);
}

int original_chicago_backend_damper(original_chicago_backend *s, unsigned value)
{
    if (!running(s)) return unavailable(s);
    if (value >= 128 || (s->block_open && s->events.next_frame >= s->block_frames)) return ORIGINAL_CHICAGO_BACKEND_INVALID;
    if (!original_chicago_scheduler_damper(&s->scheduler, value)) return fault(s);
    return healthy(s);
}

int original_chicago_backend_all_off(original_chicago_backend *s)
{
    if (!running(s)) return unavailable(s);
    original_chicago_allocator_all_notes_off(&s->allocator,
        s->scheduler_callbacks.voice, &s->voice_owner);
    return healthy(s);
}

int original_chicago_backend_filter_view(const original_chicago_backend *s, float *eg, float *depth)
{
    if (!s || s->ready != BACKEND_READY) return 0;
    *eg = s->voice.active ? s->voice.vcf.level : 0.0f;
    *depth = s->controls.env_depth;
    return 1;
}

int original_chicago_backend_active(const original_chicago_backend *s)
{
    /* Raw latched keys survive ordinary releases; only an enabled latch
     * lets the selector schedule them again. The pending descriptor also
     * survives a completed or cancelled restart. Its mask is saved data,
     * not a live voice: only the actual restart continuation is activity. */
    return s && s->ready == BACKEND_READY &&
        (s->keys.physical_count ||
         (s->keys.committed_enable && s->keys.effective_latch && s->keys.latched_count) ||
         s->voice.active || s->voice.awaiting_restart);
}

static int disjoint(const void *a, size_t an, const void *b, size_t bn)
{
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= UINTPTR_MAX - an && y <= UINTPTR_MAX - bn && (x + an <= y || y + bn <= x);
}

static int workspace_valid(const original_chicago_backend_workspace *w, uint32_t n)
{
    if (!w || !n || n > w->capacity || w->capacity > ORIGINAL_CHICAGO_BACKEND_MAX_BLOCK) return 0;
    const void *p[8] = {w->core_left,w->core_right,w->converted_left,w->converted_right,
        w->scratch[0],w->scratch[1],w->scratch[2],w->scratch[3]};
    for (unsigned i = 0; i < 8; ++i) {
        size_t width = i < 2 ? sizeof(uint32_t) : sizeof(int16_t);
        if (!p[i] || ((uintptr_t)p[i] & (width - 1))) return 0;
        for (unsigned j = 0; j < i; ++j)
            if (!disjoint(p[i], width * w->capacity, p[j],
                (j < 2 ? sizeof(uint32_t) : sizeof(int16_t)) * w->capacity)) return 0;
    }
    return 1;
}

static int outputs_valid(const original_chicago_backend_workspace *w,
    const float *left, const float *right, uint32_t n)
{
    size_t bytes = sizeof(float) * n;
    if (!left || !right || ((uintptr_t)left & 3u) || ((uintptr_t)right & 3u) ||
        !disjoint(left, bytes, right, bytes)) return 0;
    const void *p[8] = {w->core_left,w->core_right,w->converted_left,w->converted_right,
        w->scratch[0],w->scratch[1],w->scratch[2],w->scratch[3]};
    for (unsigned i = 0; i < 8; ++i) {
        size_t span = (i < 2 ? sizeof(uint32_t) : sizeof(int16_t)) * w->capacity;
        if (!disjoint(left, bytes, p[i], span) || !disjoint(right, bytes, p[i], span)) return 0;
    }
    return 1;
}

int original_chicago_backend_prepare(original_chicago_backend *s)
{
    if (!running(s)) return unavailable(s);
    if (s->block_open) return ORIGINAL_CHICAGO_BACKEND_INVALID;
    if (s->block_prepared) return ORIGINAL_CHICAGO_BACKEND_OK;
    chicago_arp_clock_prepare(&s->clock1, 44100, s->controls.mg_base[0], s->controls.mg_times[0]);
    chicago_arp_clock_prepare(&s->clock2, 44100,
        s->controls.routing.arp_base_note, s->controls.routing.arp_times);
    if (!original_chicago_scheduler_prepare(&s->scheduler) || healthy(s) != ORIGINAL_CHICAGO_BACKEND_OK)
        return fault(s);
    s->block_prepared = 1;
    return ORIGINAL_CHICAGO_BACKEND_OK;
}

static int begin_admitted(original_chicago_backend *s,
    original_chicago_backend_workspace *w, const original_engine_event *events,
    uint32_t count, uint32_t frames)
{
    original_engine_events next;
    if (!original_engine_events_bind(&next, events, count, frames, NULL, &s->control_owner))
        return ORIGINAL_CHICAGO_BACKEND_INVALID;
    /* The generic queue validates syntax; this policy also admits each FX
     * type edit before prepare or any earlier event can mutate the backend. */
    if (s->output.fx2.cold_policy) {
        for (uint32_t i = 0; i < count; ++i) {
            if (events[i].kind != ORIGINAL_EVENT_MACRO || events[i].parameter != 17) continue;
            float value = original_math_float(events[i].value);
            original_chicago_program program = s->program;
            original_chicago_routing routing;
            if (!original_chicago_program_set(&program, 17, value) ||
                !original_chicago_program_routing(&program, &routing) ||
                !original_chicago_fx2_wrapper_supports(&s->output.fx2, routing.fx[1][0]))
                return ORIGINAL_CHICAGO_BACKEND_INVALID;
        }
    }
    int prepared = original_chicago_backend_prepare(s);
    if (prepared != ORIGINAL_CHICAGO_BACKEND_OK) return prepared;
    s->events = next;
    s->workspace = w;
    s->block_frames = frames;
    s->block_open = 1;
    return ORIGINAL_CHICAGO_BACKEND_OK;
}

int original_chicago_backend_begin(original_chicago_backend *s,
    original_chicago_backend_workspace *w, const original_engine_event *events,
    uint32_t count, uint32_t frames)
{
    if (!running(s)) return unavailable(s);
    if (s->block_open || !workspace_valid(w, frames)) return ORIGINAL_CHICAGO_BACKEND_INVALID;
    return begin_admitted(s, w, events, count, frames);
}

int original_chicago_backend_frame(original_chicago_backend *s)
{
    if (!running(s)) return unavailable(s);
    if (!s->block_open || s->events.next_frame >= s->block_frames)
        return ORIGINAL_CHICAGO_BACKEND_INVALID;
    uint32_t i = s->events.next_frame;
    float left, right;
    if (!original_engine_events_frame(&s->events, i) ||
        original_chicago_scheduler_after_events(&s->scheduler) <= 0 ||
        !original_chicago_voice_owner_control_all(&s->voice_owner) ||
        !original_chicago_voice_render_frame(&s->voice, &s->controls, &left, &right) ||
        healthy(s) != ORIGINAL_CHICAGO_BACKEND_OK) return fault(s);
    s->workspace->core_left[i] = original_math_bits(left);
    s->workspace->core_right[i] = original_math_bits(right);
    return ORIGINAL_CHICAGO_BACKEND_OK;
}

static int end_admitted(original_chicago_backend *s, float *left, float *right, unsigned known_zero)
{
    original_chicago_backend_workspace *w = s->workspace;
    original_chicago_output_workspace ow = {w->converted_left,w->converted_right,
        w->scratch[0],w->scratch[1],w->capacity};
    original_chicago_fx2_workspace fw = {w->scratch[0],w->scratch[1],
        w->scratch[2],w->scratch[3],w->capacity};
    int (*output)(original_chicago_engine_output *,original_chicago_output_workspace *,
        original_chicago_fx2_workspace *,const original_chicago_routing *,const uint32_t *,
        const uint32_t *,uint32_t *,uint32_t *,uint32_t,unsigned) = known_zero ?
        original_chicago_engine_output_process_zero : original_chicago_engine_output_process;
    if (!output(&s->output, &ow, &fw, &s->controls.routing,
        w->core_left, w->core_right, w->core_left, w->core_right, s->block_frames, s->fz)) return fault(s);
    memcpy(left, w->core_left, sizeof *left * s->block_frames);
    memcpy(right, w->core_right, sizeof *right * s->block_frames);
    s->block_open = 0;
    s->block_prepared = 0;
    s->block_frames = 0;
    s->workspace = NULL;
    return ORIGINAL_CHICAGO_BACKEND_OK;
}

int original_chicago_backend_end(original_chicago_backend *s, float *left, float *right)
{
    if (!running(s)) return unavailable(s);
    if (!s->block_open || s->events.next_frame != s->block_frames ||
        !outputs_valid(s->workspace, left, right, s->block_frames)) return ORIGINAL_CHICAGO_BACKEND_INVALID;
    return end_admitted(s, left, right, 0);
}

int original_chicago_backend_render(original_chicago_backend *s,
    original_chicago_backend_workspace *w, const original_engine_event *events,
    uint32_t count, float *left, float *right, uint32_t frames)
{
    if (!running(s)) return unavailable(s);
    if (s->block_open) return ORIGINAL_CHICAGO_BACKEND_INVALID;
    if (!frames) return count ? ORIGINAL_CHICAGO_BACKEND_INVALID : ORIGINAL_CHICAGO_BACKEND_OK;
    if (!workspace_valid(w, frames) || !outputs_valid(w, left, right, frames))
        return ORIGINAL_CHICAGO_BACKEND_INVALID;
    /* The serial whole-block entry has already admitted workspace/output
     * geometry. Public begin/end remain checked for split-frame callers. */
    int result = begin_admitted(s, w, events, count, frames);
    if (result != ORIGINAL_CHICAGO_BACKEND_OK) return result;
    if (!count && !original_chicago_backend_active(s) &&
        s->allocator.voice_active.head < 0 && s->allocator.capacity == 1 &&
        s->voice.vca.state == 6 && !s->voice.awaiting_restart &&
        s->global_input.sample_rate_integer == 44100 &&
        s->global_input.sample_rate == 44100 && s->global_input.tempo_sync2 == 1 &&
        healthy(s) == ORIGINAL_CHICAGO_BACKEND_OK) {
        unsigned action;
        if (!original_chicago_voice_control(&s->voice, &s->controls, &s->tables,
                s->voice_owner.bpm, 0, &action) || action) return fault(s);
        /* Disabled voices freeze oscillator/noise/ramp/filter state in the
         * original path. Only clocks, gate and global MG continue. Split at
         * each MG update so it sees the exact clocks of that sample. The
         * output owner still drains every nonzero history or unsettled ramp. */
        uint32_t remaining = frames;
        while (remaining) {
            uint32_t n = original_chicago_global_mg_quiet_frames(&s->global, &s->global_input);
            if (n > remaining) n = remaining;
            if (n) {
                if (!original_chicago_scheduler_quiet(&s->scheduler, &s->clock1, n)) return fault(s);
                s->global.counter += (int32_t)(690u * n);
                s->global_stage.helper_entries += n;
                remaining -= n;
            }
            if (remaining) {
                if (original_chicago_scheduler_after_events(&s->scheduler) <= 0 ||
                    !original_chicago_selected_global_exit(&s->global_stage)) return fault(s);
                --remaining;
            }
        }
        if (healthy(s) != ORIGINAL_CHICAGO_BACKEND_OK) return fault(s);
        memset(w->core_left, 0, sizeof *w->core_left * frames);
        memset(w->core_right, 0, sizeof *w->core_right * frames);
        s->events.next_frame = frames;
        return end_admitted(s, left, right, 1);
    }
    if (!count && s->allocator.capacity == 1) {
        /* No event can edit controls or borrowed resources inside this call.
         * Admit the serial block once; retain sample-position clock, VCA and
         * MG ticks, including selector-driven notes and restart continuations. */
        if (!original_chicago_voice_owner_render_block(&s->voice_owner, &s->scheduler,
                w->core_left, w->core_right, frames) ||
            healthy(s) != ORIGINAL_CHICAGO_BACKEND_OK) return fault(s);
        s->events.next_frame = frames;
        return end_admitted(s, left, right, 0);
    }
    for (uint32_t i = 0; i < frames; ++i) {
        result = original_chicago_backend_frame(s);
        if (result != ORIGINAL_CHICAGO_BACKEND_OK) return result;
    }
    return end_admitted(s, left, right, 0);
}
