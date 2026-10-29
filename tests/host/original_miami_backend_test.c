/* SPDX-License-Identifier: GPL-3.0-only */
#include "../../firmware/src/dsp/original_miami_backend.h"
#include "binary32_math.h"
#include "chicago_key_admission.h"
#include "motion_tables_private.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Independent direct owner graph, following original_miami_owner_graph_test.
 * It never calls the backend to construct, dispatch or render its reference.
 * Only the cold start policy is shared: explicit init + qualified core reset. */
typedef struct {
    original_miami_program p;
    original_miami_voice_controls c;
    original_miami_output output;
    chicago_arp_control keys;
    original_held_record held[128];
    original_voice_record records[2];
    original_miami_allocator a;
    original_miami_selected_voice v[2];
    original_miami_pending_note pending[2];
    original_miami_extra_sources extra[2];
    original_chicago_global_mg global;
    original_chicago_global_mg_input gi;
    chicago_arp_clock clock1, clock2;
    original_miami_note_context context;
    original_miami_voice_owner owner;
    original_chicago_selector selector;
    chicago_arp_clock_input clock_input;
    chicago_arp_control_input control_input;
    original_chicago_scheduler_callbacks cb;
    original_chicago_scheduler scheduler;
    original_miami_control_owner control;
} direct_graph;

typedef struct {
    uint32_t left[256], right[256];
    int32_t dry[2][256];
    int16_t scratch[2][256];
} workspace_storage;

static uint64_t hash = UINT64_C(1469598103934665603);
static unsigned frames_checked, blocks_checked;

static void hash_word(uint32_t x)
{
    for (unsigned i = 0; i < 4; ++i) {
        hash = (hash ^ (uint8_t)x) * UINT64_C(1099511628211);
        x >>= 8;
    }
}

static original_miami_backend_workspace workspace(workspace_storage *s)
{
    original_miami_backend_workspace w = {s->left, s->right,
        {s->dry[0], s->dry[1], s->scratch[0], s->scratch[1], 256}};
    return w;
}

static void direct_init(direct_graph *s, const original_miami_voice_tables *tables,
    unsigned factory, float bpm)
{
    memset(s, 0, sizeof *s);
    assert(original_miami_program_init(&s->p, factory));
    assert(original_miami_voice_compile(&s->c, &s->p));
    assert(original_chicago_allocator_bind(&s->a, &s->keys, s->held, s->records, 2, 2, 2, 1, 0, 1));
    s->a.mg1_key_sync = s->a.mg2_key_sync = 1;
    s->a.arp_range = 1;
    s->a.scan_index = -1;
    s->a.previous_index = 60;
    s->a.octave = 1;
    for (unsigned i = 0; i < 2; ++i) assert(original_miami_selected_voice_init(s->v + i, tables, i, 60));
    float rate1, rate2, key1, key2, sync1, sync2;
    assert(original_miami_program_native(&s->p, 44, &rate1));
    assert(original_miami_program_native(&s->p, 50, &rate2));
    assert(original_miami_program_native(&s->p, 45, &key1));
    assert(original_miami_program_native(&s->p, 51, &key2));
    assert(original_miami_program_native(&s->p, 46, &sync1));
    assert(original_miami_program_native(&s->p, 52, &sync2));
    assert(key1 == 1 && key2 == 1 && sync1 == 1 && sync2 == 1);
    s->gi = (original_chicago_global_mg_input){44100, 44100, rate1, rate2,
        s->c.mg_shape[0], s->c.mg_shape[1], 1, 1};
    s->clock1.threshold = s->clock2.threshold = 1;
    original_miami_note_producers producers = original_miami_note_context_callbacks(&s->context);
    assert(original_miami_voice_owner_bind(&s->owner, s->v, s->pending, s->extra,
        2, &s->a, &s->c, tables, &producers, bpm));
    assert(original_miami_note_context_bind(&s->context, &s->owner, &s->global,
        &s->gi, &s->clock1, &s->clock2, 0, 0, 1, 1));
    s->selector = (original_chicago_selector){-1, 60, 1, 0, 0x6d70};
    s->clock_input = (chicago_arp_clock_input){bpm, 1};
    s->cb = original_miami_note_context_scheduler_callbacks();
    assert(original_chicago_scheduler_bind(&s->scheduler, &s->a, &s->selector,
        &s->clock2, &s->clock_input, &s->control_input, &s->cb, &s->owner));
    original_miami_output_init(&s->output);
    assert(original_miami_control_owner_bind(&s->control, &s->p, &s->c,
        &s->output, &s->scheduler, &s->gi));
    assert(original_miami_note_context_core_reset(&s->context));
    assert(original_miami_output_apply(&s->output, &s->p, 10));
    assert(original_chicago_scheduler_damper(&s->scheduler, s->c.damper ? 127 : 0));
    assert(original_miami_control_owner_block_prepare(&s->control, &s->clock1));
}

static void direct_render(direct_graph *s, original_miami_backend_workspace *w,
    unsigned n, const original_engine_event *events, unsigned count)
{
    if (!n) {
        assert(!count);
        return;
    }
    original_engine_events queue;
    assert(original_engine_events_bind(&queue, events, count, n, &s->control, NULL));
    assert(original_miami_control_owner_block_prepare(&s->control, &s->clock1));
    for (unsigned frame = 0; frame < n; ++frame) {
        assert(original_engine_events_frame(&queue, frame));
        assert(original_chicago_canonical_keys_valid(&s->keys));
        assert(original_chicago_scheduler_after_events(&s->scheduler));
        assert(original_miami_voice_owner_control_all(&s->owner));
        float left = 0, right = 0;
        for (unsigned i = 0; i < 2; ++i) {
            float x, y;
            assert(original_miami_selected_voice_render_frame(s->v + i, &s->c, &x, &y));
            left = left + x;
            right = right + y;
        }
        w->left[frame] = original_math_bits(left);
        w->right[frame] = original_math_bits(right);
    }
    assert(queue.cursor == count && s->owner.status && s->context.status && s->control.status);
    assert(original_miami_output_process(&s->output, &s->p, &w->fx,
        w->left, w->right, w->left, w->right, n, 0, NULL, 0));
}

static void same_state(const original_miami_backend *b, const direct_graph *r)
{
#define SAME(a, c) assert(!memcmp(&(a), &(c), sizeof(a)))
    SAME(b->program, r->p); SAME(b->controls, r->c); SAME(b->output, r->output);
    SAME(b->keys, r->keys); SAME(b->held, r->held); SAME(b->records, r->records);
    SAME(b->voices, r->v); SAME(b->pending, r->pending); SAME(b->extra, r->extra);
    SAME(b->global, r->global); SAME(b->global_input, r->gi);
    SAME(b->clock1, r->clock1); SAME(b->clock2, r->clock2);
    SAME(b->clock_input, r->clock_input); SAME(b->control_input, r->control_input);
    const size_t offset = offsetof(original_miami_allocator, held_active);
    assert(!memcmp((const unsigned char *)&b->allocator + offset,
        (const unsigned char *)&r->a + offset, sizeof r->a - offset));
    assert(b->selector.cursor == r->selector.cursor && b->selector.last_key == r->selector.last_key &&
        b->selector.octave == r->selector.octave && b->selector.direction == r->selector.direction &&
        b->selector.random == r->selector.random);
    assert(b->note_context.helper_entries == r->context.helper_entries &&
        b->note_context.head_seeds == r->context.head_seeds && b->voice_owner.bpm == r->owner.bpm &&
        b->scheduler.dispatch_result == r->scheduler.dispatch_result);
    assert(original_miami_backend_status(b));
#undef SAME
}

static void compare_render(original_miami_backend *b, direct_graph *r,
    original_miami_backend_workspace *bw, original_miami_backend_workspace *rw,
    unsigned n, const original_engine_event *events, unsigned count, unsigned use_float)
{
    float left[258], right[258];
    for (unsigned i = 0; i < 258; ++i) left[i] = right[i] = -123.5f;
    unsigned serial = 0;
    if (n && use_float) {
        assert(original_miami_backend_prepare(b));
        chicago_arp_clock before = b->clock1;
        assert(original_miami_backend_prepare(b));
        assert(!memcmp(&before, &b->clock1, sizeof before));
        /* Firmware consumes frame-zero input before it knows the length to
         * the next sequence boundary. Compare this serial path with the
         * reference's complete immutable event batch and identical block. */
        while (serial < count && events[serial].frame == 0) {
            const original_engine_event *e = events + serial;
            if (e->kind == ORIGINAL_EVENT_MACRO)
                assert(original_miami_backend_set_macro(b, e->parameter, original_math_float(e->value)));
            else if (e->kind == ORIGINAL_EVENT_NOTE)
                assert(e->value ? original_miami_backend_note_on(b, e->parameter, e->value) :
                                 original_miami_backend_note_off(b, e->parameter));
            else break;
            ++serial;
        }
    }
    const original_engine_event *remaining = serial ? events + serial : events;
    if (use_float)
        assert(original_miami_backend_render_events(b, bw, left + 1, right + 1, n, remaining, count - serial));
    else
        assert(original_miami_backend_render_event_words(b, &bw->fx, bw->left, bw->right, n, events, count));
    direct_render(r, rw, n, events, count);
    same_state(b, r);
    for (unsigned i = 0; i < n; ++i) {
        uint32_t l = use_float ? original_math_bits(left[i + 1]) : bw->left[i];
        uint32_t rr = use_float ? original_math_bits(right[i + 1]) : bw->right[i];
        assert(l == rw->left[i] && rr == rw->right[i]);
        hash_word(l); hash_word(rr);
    }
    assert(left[0] == -123.5f && right[0] == -123.5f && left[n + 1] == -123.5f && right[n + 1] == -123.5f);
    frames_checked += n;
    ++blocks_checked;
}

static void run_factory(unsigned factory, const original_miami_voice_tables *tables)
{
    original_miami_backend b;
    direct_graph r;
    workspace_storage bs, rs;
    original_miami_backend_workspace bw = workspace(&bs), rw = workspace(&rs);
    assert(original_miami_backend_init(&b, factory, 137));
    direct_init(&r, tables, factory, 137);
    same_state(&b, &r);
    assert(original_miami_backend_idle(&b));
    assert(original_miami_backend_program(&b) == &b.program);
    for (unsigned block = 0; block < 36; ++block) {
        unsigned note = 48 + block % 12;
        original_engine_event events[] = {
            {0, ORIGINAL_EVENT_MACRO, block / 2, original_math_bits((float)(block & 1))},
            {0, ORIGINAL_EVENT_NOTE, note, 100},
            {7, ORIGINAL_EVENT_NOTE, note + 7, 127},
            {8, ORIGINAL_EVENT_NOTE, note, 0},
            {9, ORIGINAL_EVENT_DAMPER, 0, 127},
            {10, ORIGINAL_EVENT_NOTE, note + 7, 0},
            {10, ORIGINAL_EVENT_NOTE, note + 12, 97},
            {63, ORIGINAL_EVENT_DAMPER, 0, 0},
            {64, ORIGINAL_EVENT_MACRO, 13, original_math_bits((float)(block % 8) / 7.0f)},
            {128, ORIGINAL_EVENT_NOTE, note + 12, 0},
            {190, ORIGINAL_EVENT_NOTE, note, 0},
            {210, ORIGINAL_EVENT_CLEAR_KEYS, 0, 0},
            {220, ORIGINAL_EVENT_ALL_OFF, 0, 0},
            {240, ORIGINAL_EVENT_MUTE, 0, block & 1}
        };
        compare_render(&b, &r, &bw, &rw, 256, events, sizeof events / sizeof *events, block & 1);
    }
    /* Direct API is explicitly between blocks; every earlier release reaches
     * the owner, and release audio continues after held-key count reaches0. */
    assert(original_miami_backend_set_macro(&b, 17, 0));
    assert(original_miami_control_owner_edit(&r.control, 17, 0));
    assert(original_miami_backend_note_on(&b, 60, 99));
    assert(original_chicago_scheduler_note(&r.scheduler, 60, 99));
    assert(original_miami_backend_note_on(&b, 64, 100));
    assert(original_chicago_scheduler_note(&r.scheduler, 64, 100));
    assert(original_miami_backend_note_off(&b, 60));
    assert(original_chicago_scheduler_note(&r.scheduler, 60, 0));
    assert(!b.keys.physical[60] && b.keys.physical[64] == 100);
    static const unsigned sizes[] = {1, 2, 7, 16, 31, 64, 127, 255, 256};
    for (unsigned i = 0; i < sizeof sizes / sizeof *sizes; ++i) {
        float bpm = i & 1 ? 20 : 400;
        assert(original_miami_backend_set_tempo(&b, bpm));
        r.owner.bpm = r.clock_input.bpm = bpm;
        compare_render(&b, &r, &bw, &rw, sizes[i], NULL, 0, i & 1);
    }
    assert(original_miami_backend_note_off(&b, 64));
    assert(original_chicago_scheduler_note(&r.scheduler, 64, 0));
    assert(!b.keys.physical_count && original_miami_backend_active(&b));
    compare_render(&b, &r, &bw, &rw, 256, NULL, 0, 1);

    /* Factory replacement keeps original float values and live history. */
    unsigned next = (factory + 1) % 30;
    assert(original_miami_backend_load_factory(&b, next));
    assert(original_miami_program_init(&r.p, next) && original_miami_voice_compile(&r.c, &r.p));
    r.gi.waveform1 = r.c.mg_shape[0]; r.gi.waveform2 = r.c.mg_shape[1];
    assert(original_miami_output_apply(&r.output, &r.p, 10));
    assert(original_chicago_scheduler_damper(&r.scheduler, r.c.damper ? 127 : 0));
    same_state(&b, &r);
    assert(original_miami_backend_transport(&b, 1));
    assert(original_miami_backend_note_on(&b, 67, 127));
    assert(original_chicago_scheduler_note(&r.scheduler, 67, 127));
    compare_render(&b, &r, &bw, &rw, 64, NULL, 0, 0);
    assert(original_miami_backend_transport(&b, 0));
    original_chicago_allocator_all_notes_off(&r.a, original_miami_voice_owner_callbacks(), &r.owner);
    same_state(&b, &r);
    assert(original_miami_backend_transport(&b, 0));
    same_state(&b, &r);
    compare_render(&b, &r, &bw, &rw, 127, NULL, 0, 1);
    assert(original_miami_backend_panic(&b));
    assert(original_miami_note_context_core_reset(&r.context));
    original_miami_fx_reset(&r.output.fx);
    same_state(&b, &r);
    assert(original_miami_backend_idle(&b));
    uint32_t old_helpers = b.note_context.helper_entries;
    compare_render(&b, &r, &bw, &rw, 64, NULL, 0, 0);
    assert(b.note_context.helper_entries > old_helpers && original_miami_backend_idle(&b));
}

static void rejected_inputs(void)
{
    original_miami_backend b;
    workspace_storage storage;
    original_miami_backend_workspace w = workspace(&storage);
    float left[256], right[256];
    assert(original_miami_backend_init(&b, 0, 120));
    unsigned char saved[sizeof b];
#define REJECT(call) do { memcpy(saved, &b, sizeof b); assert(!(call)); assert(!memcmp(saved, &b, sizeof b)); } while (0)
    REJECT(original_miami_backend_init(&b, 30, 120));
    REJECT(original_miami_backend_init(&b, 0, 19));
    REJECT(original_miami_backend_load_factory(&b, 30));
    REJECT(original_miami_backend_set_macro(&b, 18, 0));
    REJECT(original_miami_backend_set_macro(&b, 0, -1));
    REJECT(original_miami_backend_set_macro(&b, 0, original_math_float(0x7fc00000u)));
    REJECT(original_miami_backend_set_tempo(&b, 401));
    REJECT(original_miami_backend_set_tempo(&b, original_math_float(0x7f800000u)));
    REJECT(original_miami_backend_note_on(&b, 128, 100));
    REJECT(original_miami_backend_note_on(&b, 60, 0));
    REJECT(original_miami_backend_note_on(&b, 60, 128));
    REJECT(original_miami_backend_note_off(&b, 128));
    REJECT(original_miami_backend_damper(&b, 128));
    REJECT(original_miami_backend_transport(&b, 2));
    REJECT(original_miami_backend_render_stereo(&b, &w, left, right, 257));
    REJECT(original_miami_backend_render_stereo(&b, &w, left, left + 1, 16));
    REJECT(original_miami_backend_render_stereo(&b, &w, &b.program.macro[0], right, 16));
    REJECT(original_miami_backend_render_words(&b, &w.fx, storage.left, storage.left + 1, 16));
    original_engine_event events[2] = {{0, ORIGINAL_EVENT_NOTE, 60, 100}, {1, ORIGINAL_EVENT_MACRO, 0, 0x7fc00000u}};
    REJECT(original_miami_backend_render_events(&b, &w, left, right, 16, events, 2));
    events[1] = (original_engine_event){16, ORIGINAL_EVENT_NOTE, 60, 0};
    REJECT(original_miami_backend_render_events(&b, &w, left, right, 16, events, 2));
    events[0].frame = 1; events[1].frame = 0;
    REJECT(original_miami_backend_render_events(&b, &w, left, right, 16, events, 2));
    events[0] = (original_engine_event){0, ORIGINAL_EVENT_MUTE + 1, 0, 0};
    REJECT(original_miami_backend_render_events(&b, &w, left, right, 16, events, 1));
    REJECT(original_miami_backend_render_events(&b, &w, left, right, 16, NULL, 1));
    REJECT(original_miami_backend_render_events(&b, &w, left, right, 0, events, 1));
    memcpy(saved, &b, sizeof b);
    assert(original_miami_backend_render_events(&b, NULL, NULL, NULL, 0, NULL, 0));
    assert(!memcmp(saved, &b, sizeof b));
    original_miami_fx_workspace short_w = w.fx; short_w.capacity = 15;
    REJECT(original_miami_backend_render_words(&b, &short_w, storage.left, storage.right, 16));
    original_miami_fx_workspace aliased = w.fx; aliased.dryR = aliased.dryL;
    REJECT(original_miami_backend_render_words(&b, &aliased, storage.left, storage.right, 16));
#undef REJECT
    /* Failure is observable and sticky; float outputs do not become success-
     * shaped zeros. Init is the explicit recovery path. */
    for (unsigned i = 0; i < 256; ++i) left[i] = right[i] = 1.25f;
    b.controls.ready = 0;
    assert(!original_miami_backend_render_stereo(&b, &w, left, right, 16));
    assert(!original_miami_backend_status(&b) && !original_miami_backend_idle(&b));
    assert(!original_miami_backend_note_on(&b, 60, 100));
    for (unsigned i = 0; i < 256; ++i) assert(left[i] == 1.25f && right[i] == 1.25f);
    assert(original_miami_backend_init(&b, 0, 120) && original_miami_backend_status(&b));
}

int main(void)
{
    const original_ms20_tables filter = {original_miami_a44100, original_miami_b44100,
        original_miami_r, original_miami_cap};
    original_miami_voice_tables tables;
    assert(original_miami_voice_tables_bind_immutable(&tables, &filter, original_pitch44100));
    for (unsigned factory = 0; factory < 30; ++factory) run_factory(factory, &tables);
    rejected_inputs();
    printf("PASS original Miami firmware adapter vs direct selected owner graph: 30 factories, 18 endpoints, %u stereo frames, %u blocks, hash %016llx; keyed releases, ordered events, tempo, tails, factory/transport/panic, rejected inputs; explicit firmware cold start, no plugin startup claim\n",
        frames_checked, blocks_checked, (unsigned long long)hash);
    return 0;
}
