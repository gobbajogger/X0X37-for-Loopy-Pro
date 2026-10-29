/* SPDX-License-Identifier: GPL-3.0-only */
#include "original_chicago_backend.h"
#ifdef ORIGINAL_CHICAGO_BACKEND_LAYOUT_ONLY
const uint32_t original_chicago_backend_layout[] = {
    sizeof(original_chicago_backend), sizeof(original_chicago_backend_workspace),
    sizeof(original_chicago_voice), sizeof(original_chicago_program),
    sizeof(chicago_arp_control), sizeof(original_held_record) * 128,
    sizeof(original_chicago_engine_output)
};
#else
#include "chicago_fx2_bank.h"
#include "chicago_key_admission.h"
#include "chicago_packed.h"
#include "motion_tables_private.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 256u
#define OK ORIGINAL_CHICAGO_BACKEND_OK
static uint64_t pre_hash = UINT64_C(1469598103934665603);
static uint64_t output_hash = UINT64_C(1469598103934665603);
static unsigned checked_frames, key_events, stereo_output_frames;

static void hash_word(uint64_t *hash, uint32_t u)
{
    for (unsigned i = 0; i < 4; ++i) { *hash = (*hash ^ (uint8_t)u) * UINT64_C(1099511628211); u >>= 8; }
}

static uint32_t bits(float x) { uint32_t u; memcpy(&u, &x, 4); return u; }

/* Real12 selected algorithms are used only as a host projection fixture.
 * Their dedicated histories/public reset are deliberately NOT presented as
 * the full original shared-arena provider required for firmware admission.
 * The exact proof below concerns pre-FX state/audio and adapter ordering;
 * post-FX comparison proves wiring under this named projection, not aliases. */
typedef struct {
    original_chicago_fx2_bank bank;
    original_chicago_fx2_storage storage;
    original_chicago_fx2_owner owner;
} projection_fixture;

static void *memory(size_t bytes)
{
    void *p = malloc(bytes); assert(p); memset(p, 0xa5, bytes); return p;
}

static void projection_init(projection_fixture *p)
{
    memset(p, 0, sizeof *p);
    p->storage = (original_chicago_fx2_storage){
        {memory(2 * CHFX_SHORT_SLOTS),CHFX_SHORT_SLOTS},
        {memory(2 * CHFX_BPM_STEREO_SLOTS),CHFX_BPM_STEREO_SLOTS},
        {memory(2 * CHFX_GATE_SLOTS),CHFX_GATE_SLOTS},
        {memory(2 * CHFX_HALL_SLOTS),CHFX_HALL_SLOTS},
        {memory(4096),2048},memory(4096),memory(4096),1024,{NULL,NULL}};
    assert(original_chicago_fx2_bank_init(&p->bank, &p->storage));
    p->owner = (original_chicago_fx2_owner){&p->bank,
        original_chicago_fx2_bank_find,original_chicago_fx2_bank_reset_public_projection};
}

static void projection_free(projection_fixture *p)
{
    free(p->storage.short_delay.data); free(p->storage.bpm_delay.data);
    free(p->storage.gate.data); free(p->storage.hall.data); free(p->storage.chorus.data);
    free(p->storage.ensemble_left); free(p->storage.ensemble_right);
}

typedef struct {
    uint32_t core[2][N];
    int16_t converted[2][N], scratch[4][N];
    float out[2][N];
    original_chicago_backend_workspace w;
} workspace;

static void workspace_init(workspace *w)
{
    memset(w, 0xa5, sizeof *w);
    w->w = (original_chicago_backend_workspace){w->core[0],w->core[1],
        w->converted[0],w->converted[1],
        {w->scratch[0],w->scratch[1],w->scratch[2],w->scratch[3]},N};
}

/* Independently composed original APIs, following the established selected
 * graph fixture. It does not call the firmware backend for init/control/audio. */
typedef struct {
    original_chicago_program program;
    original_chicago_voice_controls controls;
    original_chicago_voice_tables tables;
    original_chicago_voice voice;
    original_pending_note pending;
    original_chicago_extra_sources extra;
    chicago_arp_control keys;
    original_held_record held[128];
    original_voice_record record;
    original_chicago_allocator allocator;
    original_chicago_global_mg global;
    original_chicago_global_mg_input global_input;
    chicago_arp_clock clock1,clock2;
    original_chicago_selected_global_stage global_stage;
    original_chicago_voice_owner voice_owner;
    original_chicago_note_context note_context;
    original_chicago_selector selector;
    chicago_arp_clock_input clock_input;
    chicago_arp_control_input control_input;
    original_chicago_scheduler_callbacks callbacks;
    original_chicago_scheduler scheduler;
    original_chicago_engine_output output;
    original_chicago_control_owner control;
    original_engine_events events;
} reference;

static void reference_init(reference *r, unsigned factory, float bpm,
    original_chicago_tuning *tuning, const original_chicago_fx2_owner *effects)
{
    memset(r, 0, sizeof *r);
    assert(original_chicago_program_init(&r->program, factory));
    assert(original_chicago_voice_compile(&r->controls, &r->program));
    assert(original_chicago_voice_tables_bind(&r->tables,
        original_chicago_packed_lookup, NULL, original_pitch44100));
    original_chicago_voice_seed seed; memset(&seed, 0, sizeof seed);
    seed.noise = 0x6d70; seed.last_pitch = 60;
    seed.mg[0].phase = seed.mg[1].phase = -1;
    seed.mg[0].noise = seed.mg[1].noise = 0x6d70;
    assert(original_chicago_voice_init(&r->voice, &r->tables, &seed));
    assert(original_chicago_allocator_bind(&r->allocator, &r->keys,
        r->held, &r->record, 1, 1, 1, 1, 0, 0));
    r->allocator.scan_index = -1; r->allocator.previous_index = 60;
    r->allocator.octave = 1; r->allocator.mg2_key_sync = 1;
    r->allocator.arp_mode = r->controls.routing.arp_mode;
    r->allocator.arp_range = r->controls.routing.arp_range;
    r->global.mg1.phase = r->global.mg2.phase = -1;
    r->global.mg1.random = r->global.mg2.random = 0x6d70;
    r->global_input = (original_chicago_global_mg_input){44100,44100,
        r->controls.mg_frequency[0],r->controls.mg_frequency[1],
        r->controls.mg_shape[0],r->controls.mg_shape[1],
        r->controls.mg_tempo[0],r->controls.mg_tempo[1]};
    chicago_arp_clock_prepare(&r->clock1, 44100, r->controls.mg_base[0], r->controls.mg_times[0]);
    chicago_arp_clock_prepare(&r->clock2, 44100,
        r->controls.routing.arp_base_note, r->controls.routing.arp_times);
    assert(original_chicago_selected_global_bind(&r->global_stage, &r->global,
        &r->global_input, &r->clock1, &r->clock2, &r->allocator));
    original_chicago_note_producers producers = original_chicago_note_context_callbacks(&r->note_context);
    assert(original_chicago_voice_owner_bind(&r->voice_owner, &r->voice,
        &r->pending, 1, &r->allocator, &r->controls, &r->tables, &producers, bpm));
    assert(original_chicago_note_context_bind(&r->note_context, &r->voice_owner,
        tuning, &r->global_stage, &r->extra, 0, 1));
    r->selector = (original_chicago_selector){-1,60,1,0,0x6d70};
    r->clock_input = (chicago_arp_clock_input){bpm,r->controls.routing.arp_tempo_sync};
    r->control_input = (chicago_arp_control_input){r->controls.routing.arp_enable,
        r->controls.routing.arp_latch,r->controls.routing.damper < 64 ? 0 : 127};
    r->callbacks = original_chicago_note_context_scheduler_callbacks();
    assert(original_chicago_scheduler_bind(&r->scheduler, &r->allocator, &r->selector,
        &r->clock2, &r->clock_input, &r->control_input, &r->callbacks, &r->voice_owner));
    effects->reset_all(effects->context);
    assert(original_chicago_engine_output_init(&r->output, &r->controls.routing, effects));
    assert(original_chicago_fx2_wrapper_tempo(&r->output.fx2, (int32_t)(bpm * 10.0f)));
    original_chicago_output_mute(&r->output.output, 0);
    assert(original_chicago_control_owner_bind(&r->control, &r->program,
        &r->controls, &r->output, &r->scheduler, &r->global_input));
}

static void compare_state(const original_chicago_backend *s, const reference *r)
{
#define SAME(field) assert(!memcmp(&s->field, &r->field, sizeof s->field))
    SAME(program); SAME(controls); SAME(voice); SAME(pending); SAME(extra);
    SAME(keys); SAME(held); SAME(record); SAME(global); SAME(global_input);
    SAME(clock1); SAME(clock2); SAME(selector); SAME(clock_input); SAME(control_input);
#undef SAME
    original_chicago_allocator a = s->allocator, b = r->allocator;
    a.keys = b.keys = NULL; a.held = b.held = NULL; a.voice = b.voice = NULL;
    assert(!memcmp(&a, &b, sizeof a));
    assert(s->global_stage.helper_entries == r->global_stage.helper_entries);
    assert(s->global_stage.full_head_seeds == r->global_stage.full_head_seeds);
    assert(s->voice_owner.status && r->voice_owner.status && s->note_context.status && r->note_context.status);
    assert(original_chicago_canonical_keys_valid(&s->keys));
}

static void reference_begin(reference *r, const original_engine_event *e, unsigned count, unsigned n)
{
    assert(original_engine_events_bind(&r->events, e, count, n, NULL, &r->control));
    chicago_arp_clock_prepare(&r->clock1, 44100, r->controls.mg_base[0], r->controls.mg_times[0]);
    chicago_arp_clock_prepare(&r->clock2, 44100,
        r->controls.routing.arp_base_note, r->controls.routing.arp_times);
    assert(original_chicago_scheduler_prepare(&r->scheduler));
}

static void compare_frame(original_chicago_backend *s, reference *r, workspace *a, workspace *b, unsigned i)
{
    assert(original_engine_events_frame(&r->events, i));
    assert(original_chicago_scheduler_after_events(&r->scheduler) > 0);
    assert(original_chicago_voice_owner_control_all(&r->voice_owner));
    float l,rr; assert(original_chicago_voice_render_frame(&r->voice, &r->controls, &l, &rr));
    b->core[0][i] = bits(l); b->core[1][i] = bits(rr);
    assert(original_chicago_backend_frame(s) == OK);
    assert(a->core[0][i] == b->core[0][i] && a->core[1][i] == b->core[1][i]);
    hash_word(&pre_hash, a->core[0][i]); hash_word(&pre_hash, a->core[1][i]);
    compare_state(s, r); checked_frames++;
}

static void compare_end(original_chicago_backend *s, reference *r, workspace *a, workspace *b, unsigned n)
{
    original_chicago_output_workspace ow = {b->converted[0],b->converted[1],b->scratch[0],b->scratch[1],N};
    original_chicago_fx2_workspace fw = {b->scratch[0],b->scratch[1],b->scratch[2],b->scratch[3],N};
    assert(original_chicago_engine_output_process(&r->output, &ow, &fw, &r->controls.routing,
        b->core[0],b->core[1],b->core[0],b->core[1],n,0));
    assert(original_chicago_backend_end(s, a->out[0], a->out[1]) == OK);
    assert(!memcmp(a->out[0],b->core[0],n * 4) && !memcmp(a->out[1],b->core[1],n * 4));
    for (unsigned i = 0; i < n; ++i) {
        hash_word(&output_hash, bits(a->out[0][i])); hash_word(&output_hash, bits(a->out[1][i]));
        stereo_output_frames += bits(a->out[0][i]) != bits(a->out[1][i]);
    }
    assert(!memcmp(&s->output.output, &r->output.output, sizeof s->output.output));
}

static void factories(projection_fixture *fa, projection_fixture *fb)
{
    original_chicago_backend *s = memory(sizeof *s);
    reference *r = memory(sizeof *r);
    workspace a,b; workspace_init(&a); workspace_init(&b);
    original_chicago_tuning ta,tb;
    assert(original_chicago_tuning_bind(&ta,0,0,NULL,0x6d70)); tb = ta;
    for (unsigned factory = 0; factory < 81; ++factory) {
        assert(original_chicago_fx2_bank_init(&fa->bank,&fa->storage));
        assert(original_chicago_fx2_bank_init(&fb->bank,&fb->storage));
        assert(original_chicago_backend_init(s,factory,137,0,&ta,&fa->owner) == OK);
        reference_init(r,factory,137,&tb,&fb->owner);
        assert(!strcmp(original_chicago_backend_factory_name(factory), original_chicago_program_name(factory)));
        compare_state(s,r);
        for (unsigned edit = 0; edit < 44; ++edit) {
            original_engine_event events[] = {
                {0,ORIGINAL_EVENT_MACRO,edit/2,bits((float)(edit&1))},
                {0,ORIGINAL_EVENT_NOTE,60+edit%12,100},
                {19,ORIGINAL_EVENT_NOTE,60+edit%12,0}
            };
            if (edit & 1u) {
                assert(original_chicago_backend_prepare(s) == OK);
                chicago_arp_clock before = s->clock1;
                assert(original_chicago_backend_prepare(s) == OK);
                assert(!memcmp(&before, &s->clock1, sizeof before));
                assert(original_chicago_backend_set(s, edit/2, (float)(edit&1)) == OK);
                assert(original_chicago_backend_note(s, 60+edit%12, 100) == OK);
                assert(original_chicago_backend_begin(s,&a.w,events+2,1,31) == OK);
            } else assert(original_chicago_backend_begin(s,&a.w,events,3,31) == OK);
            reference_begin(r,events,3,31);
            for (unsigned i = 0; i < 31; ++i) compare_frame(s,r,&a,&b,i);
            compare_end(s,r,&a,&b,31);
            key_events += 2;
        }
        assert(ta.random == 0x6d70 && tb.random == 0x6d70);
    }
    free(s); free(r);
}

static void key_lifecycle(projection_fixture *fa, projection_fixture *fb)
{
    original_chicago_backend *s = memory(sizeof *s);
    reference *r = memory(sizeof *r);
    workspace a,b; workspace_init(&a); workspace_init(&b);
    original_chicago_tuning ta,tb;
    assert(original_chicago_tuning_bind(&ta,0,0,NULL,0x5678)); tb = ta;
    assert(original_chicago_fx2_bank_init(&fa->bank,&fa->storage));
    assert(original_chicago_fx2_bank_init(&fb->bank,&fb->storage));
    assert(original_chicago_backend_init(s,0,20,0,&ta,&fa->owner) == OK);
    reference_init(r,0,20,&tb,&fb->owner);
    /* ARP off, all keys press/release, duplicate records, overlapping notes,
     * pedal release, and both extreme admitted tempos. No global gate proxy. */
    assert(original_chicago_backend_set(s,13,0) == OK);
    assert(original_chicago_control_owner_edit(&r->control,13,0));
    for (unsigned key = 0; key < 128; ++key) {
        assert(original_chicago_backend_begin(s,&a.w,NULL,0,N) == OK);
        reference_begin(r,NULL,0,N);
        assert(original_chicago_backend_note(s,key,127) == OK);
        assert(original_chicago_scheduler_note(&r->scheduler,(int32_t)key,127)); key_events++;
        for (unsigned i = 0; i < N; ++i) {
            if (i == 3) {
                assert(original_chicago_backend_note(s,key,100) == OK);
                assert(original_chicago_scheduler_note(&r->scheduler,(int32_t)key,100)); key_events++;
            }
            if (i == 5) {
                unsigned next=(key+7)%128;
                assert(original_chicago_backend_note(s,next,1) == OK);
                assert(original_chicago_scheduler_note(&r->scheduler,(int32_t)next,1)); key_events++;
            }
            if (i == 13) {
                assert(original_chicago_backend_damper(s,127) == OK);
                assert(original_chicago_scheduler_damper(&r->scheduler,127));
            }
            if (i == 17 || i == 23) {
                unsigned released = i == 17 ? key : (key+7)%128;
                assert(original_chicago_backend_release(s,released) == OK);
                assert(original_chicago_scheduler_note(&r->scheduler,(int32_t)released,0)); key_events++;
            }
            if (i == 31) {
                assert(original_chicago_backend_damper(s,0) == OK);
                assert(original_chicago_scheduler_damper(&r->scheduler,0));
            }
            compare_frame(s,r,&a,&b,i);
        }
        compare_end(s,r,&a,&b,N);
        if (key == 63) {
            assert(original_chicago_backend_tempo(s,400) == OK);
            r->voice_owner.bpm=r->clock_input.bpm=400;
            assert(original_chicago_fx2_wrapper_tempo(&r->output.fx2,4000));
        }
    }
    assert(original_chicago_backend_all_off(s) == OK);
    original_chicago_allocator_all_notes_off(&r->allocator,r->callbacks.voice,&r->voice_owner);
    compare_state(s,r);
    assert(!s->keys.physical_count && !s->keys.latched_count && ta.random == 0x5678);
    assert(original_chicago_backend_reset(s,80) == OK);
    assert(!s->voice.active && !s->keys.physical_count && !s->keys.latched_count);
    assert(ta.random == 0x5678 && !original_chicago_backend_active(s));
    assert(s->voice_owner.bpm == 400 && s->factory == 80);
    free(s); free(r);
}

static void refusals(projection_fixture *fx)
{
    original_chicago_backend s,saved;
    workspace w; workspace_init(&w);
    original_chicago_tuning tuning;
    assert(original_chicago_tuning_bind(&tuning,0,0,NULL,0x6d70));
    memset(&s,0xa5,sizeof s); saved=s;
    assert(original_chicago_backend_init(&s,0,120,0,&tuning,NULL) == ORIGINAL_CHICAGO_BACKEND_NEEDS_FX_OWNER);
    assert(!memcmp(&s,&saved,sizeof s));
    original_chicago_fx2_owner bad=fx->owner; bad.reset_all=NULL;
    assert(original_chicago_backend_init(&s,0,120,0,&tuning,&bad) == ORIGINAL_CHICAGO_BACKEND_NEEDS_FX_OWNER);
    assert(!memcmp(&s,&saved,sizeof s));
    assert(original_chicago_backend_init(&s,0,120,0,&tuning,&fx->owner) == OK);
    saved=s;
    assert(original_chicago_backend_init(&s,81,120,0,&tuning,&fx->owner) == 0);
    assert(original_chicago_backend_init(&s,0,NAN,0,&tuning,&fx->owner) == 0);
    assert(original_chicago_backend_init(&s,0,120,2,&tuning,&fx->owner) == 0);
    assert(original_chicago_backend_set(&s,22,0) == 0);
    assert(original_chicago_backend_set(&s,0,NAN) == 0);
    assert(original_chicago_backend_set(&s,0,-1) == 0);
    assert(original_chicago_backend_tempo(&s,19) == 0);
    assert(original_chicago_backend_note(&s,60,100) == 0);
    assert(original_chicago_backend_release(&s,60) == 0);
    assert(!memcmp(&s,&saved,sizeof s));
    assert(original_chicago_backend_render(&s,NULL,NULL,0,NULL,NULL,0) == OK);
    assert(!memcmp(&s,&saved,sizeof s));
    original_engine_event unsorted[]={{3,ORIGINAL_EVENT_NOTE,60,100},{2,ORIGINAL_EVENT_NOTE,60,0}};
    assert(original_chicago_backend_begin(&s,&w.w,unsorted,2,N) == 0);
    assert(!memcmp(&s,&saved,sizeof s));
    original_engine_event bad_note={0,ORIGINAL_EVENT_NOTE,128,1};
    assert(original_chicago_backend_begin(&s,&w.w,&bad_note,1,N) == 0);
    assert(!memcmp(&s,&saved,sizeof s));
    original_chicago_backend_workspace badw=w.w; badw.scratch[3]=badw.scratch[2];
    assert(original_chicago_backend_begin(&s,&badw,NULL,0,N) == 0);
    badw=w.w; badw.capacity=N-1;
    assert(original_chicago_backend_begin(&s,&badw,NULL,0,N) == 0);
    assert(!memcmp(&s,&saved,sizeof s));
    assert(original_chicago_backend_render(&s,&w.w,NULL,0,w.out[0],w.out[0],N) == 0);
    assert(!memcmp(&s,&saved,sizeof s));
    assert(original_chicago_backend_begin(&s,&w.w,NULL,0,1) == OK);
    saved=s;
    assert(original_chicago_backend_end(&s,w.out[0],w.out[1]) == 0);
    assert(original_chicago_backend_begin(&s,&w.w,NULL,0,1) == 0);
    assert(original_chicago_backend_reset(&s,0) == 0);
    assert(original_chicago_backend_note(&s,128,0) == 0);
    assert(!memcmp(&s,&saved,sizeof s));
    assert(original_chicago_backend_frame(&s) == OK);
    saved=s;
    assert(original_chicago_backend_frame(&s) == 0);
    assert(original_chicago_backend_end(&s,w.out[0],w.out[0]) == 0);
    assert(!memcmp(&s,&saved,sizeof s));
    assert(original_chicago_backend_end(&s,w.out[0],w.out[1]) == OK);
    /* A genuine downstream failure must stop publication, retain fault, and
     * recover only through explicit cold reset. No silence reported as OK. */
    assert(original_chicago_backend_begin(&s,&w.w,NULL,0,1) == OK);
    s.voice.ready=0;
    float previous[2][N]; memcpy(previous,w.out,sizeof previous);
    assert(original_chicago_backend_frame(&s) == ORIGINAL_CHICAGO_BACKEND_FAULT);
    assert(s.status == ORIGINAL_CHICAGO_BACKEND_FAULT);
    assert(!s.workspace && !s.block_open && !s.block_prepared && !s.events.events);
    assert(original_chicago_backend_end(&s,w.out[0],w.out[1]) == ORIGINAL_CHICAGO_BACKEND_FAULT);
    assert(!memcmp(previous,w.out,sizeof previous));
    assert(original_chicago_backend_render(&s,&w.w,NULL,0,w.out[0],w.out[1],1) == ORIGINAL_CHICAGO_BACKEND_FAULT);
    assert(original_chicago_backend_reset(&s,0) == OK);
    assert(original_chicago_backend_render(&s,&w.w,NULL,0,w.out[0],w.out[1],1) == OK);
    const original_chicago_fx2_kernel *hall = fx->owner.find(fx->owner.context, 15);
    assert(hall);
    hall->begin(hall->context);
    s.output.fx2.active = hall;
    assert(fx->bank.hall.core_active);
    assert(original_chicago_backend_retire(&s) == OK);
    assert(!fx->bank.hall.core_active && !s.workspace && !s.block_open && !s.ready);
    assert(original_chicago_backend_render(&s,&w.w,NULL,0,w.out[0],w.out[1],1) == 0);
    assert(original_chicago_backend_init(&s,0,120,0,&tuning,&fx->owner) == OK);
    assert(!original_chicago_backend_factory_name(81));
}

int main(void)
{
    projection_fixture a,b; projection_init(&a); projection_init(&b);
    factories(&a,&b); key_lifecycle(&a,&b); refusals(&a);
    projection_free(&a); projection_free(&b);
    assert(stereo_output_frames);
    printf("PASS original Chicago firmware adapter:81 factories/22 endpoint pairs,128 physical key lifecycles,%u key events,%u exact pre-FX stereo frames,31/256-frame blocks,state/order/refusal/reset checks;preFX=%016llx;dedicated-history output projection=%016llx,%u unequal-L/R frames;full original FX alias admission NOT established\n",
        key_events,checked_frames,(unsigned long long)pre_hash,(unsigned long long)output_hash,stereo_output_frames);
    return 0;
}
#endif
