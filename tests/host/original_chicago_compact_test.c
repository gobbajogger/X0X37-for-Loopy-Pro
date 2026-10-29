/* SPDX-License-Identifier: GPL-3.0-only */
#include "original_chicago_backend.h"
#include "chicago_fx2_compact.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define N 4096u
#define OK ORIGINAL_CHICAGO_BACKEND_OK
static const unsigned types[] = {0,4,5,10,18,22,24};
static const unsigned macro_indices[] = {0,5,7,4,2,3,1};
static uint64_t kernel_hash = UINT64_C(1469598103934665603);
static uint64_t backend_hash = UINT64_C(1469598103934665603);
static unsigned comparisons, backend_frames, nonzero_frames, stereo_frames;

static void hash_bytes(uint64_t *h,const void *p,size_t n)
{
    const unsigned char *b = p;
    for (size_t i = 0; i < n; ++i) *h = (*h ^ b[i]) * UINT64_C(1099511628211);
}

static uint32_t bits(float x) { uint32_t u; memcpy(&u,&x,4); return u; }

/* Independent selected-kernel reference. These are separate, direct original
 * objects, not the compact union and not a claimed all30/shared-arena owner. */
typedef struct {
    original_chicago_chorus chorus;
    original_chicago_phaser phaser;
    original_chicago_ring ring;
    original_chicago_mastercomp master;
    original_chicago_decimator decimator;
    original_chicago_peaking eq;
    int16_t history[2][1024];
    original_chicago_fx2_kernel kernel;
    unsigned type;
} direct;

static void direct_reset(void *p)
{
    direct *d = p;
    switch (d->type) {
    case 4: original_chicago_chorus_reset(&d->chorus); break;
    case 5: original_chicago_phaser_reset(&d->phaser); break;
    case 10: original_chicago_ring_reset(&d->ring); break;
    case 18: original_chicago_mastercomp_reset(&d->master); break;
    case 22: original_chicago_decimator_reset(&d->decimator); break;
    case 24: original_chicago_peaking_reset(&d->eq); break;
    default: assert(d->type == 0); break;
    }
}

static void direct_parameter(void *p,unsigned id,unsigned v)
{
    direct *d = p;
    switch (d->type) {
    case 0: (void)original_chicago_bypass_parameter(id,(int32_t)v); break;
    case 4: assert(original_chicago_chorus_set(&d->chorus,id,v)); break;
    case 5: assert(original_chicago_phaser_set(&d->phaser,id,v)); break;
    case 10: assert(original_chicago_ring_set(&d->ring,id,v)); break;
    case 18: assert(original_chicago_mastercomp_set(&d->master,id-2,(int32_t)v)); break;
    case 22: assert(original_chicago_decimator_set(&d->decimator,id-2,(int32_t)v)); break;
    case 24: assert(original_chicago_peaking_set(&d->eq,id-2,v)); break;
    default: assert(0); break;
    }
}

static void direct_process(void *p,int16_t *l,int16_t *r,uint32_t n)
{
    direct *d = p;
    switch (d->type) {
    case 0: assert(original_chicago_bypass_process(l,r,n)); break;
    case 4: assert(original_chicago_chorus_process(&d->chorus,l,r,n)); break;
    case 5: assert(original_chicago_phaser_process(&d->phaser,l,r,n)); break;
    case 10: assert(original_chicago_ring_process(&d->ring,l,r,n)); break;
    case 18: assert(original_chicago_mastercomp_process(&d->master,l,r,n)); break;
    case 22: assert(original_chicago_decimator_process(&d->decimator,l,r,n)); break;
    case 24: assert(original_chicago_peaking_process(&d->eq,l,r,n)); break;
    default: assert(0); break;
    }
}

static void direct_activity(void *p,unsigned value)
{
    direct *d = p;
    switch (d->type) {
    case 4: d->chorus.activity = (uint8_t)value; break;
    case 5: d->phaser.activity = (uint8_t)value; break;
    case 10: d->ring.activity = (uint8_t)value; break;
    case 18: original_chicago_mastercomp_activity(&d->master,(uint8_t)value); break;
    case 24: original_chicago_peaking_activity(&d->eq,(uint8_t)value); break;
    default: break;
    }
}

static void noop(void *p) { (void)p; }
static void noop_tempo(void *p,int32_t value) { (void)p; (void)value; }
static const original_chicago_fx2_kernel *direct_find(void *p,unsigned type)
{
    direct *d = p;
    return type == d->type ? &d->kernel : NULL;
}

static void direct_init(direct *d,unsigned type)
{
    memset(d,0xa5,sizeof *d);
    d->type = type;
    switch (type) {
    case 0: break;
    case 4: assert(original_chicago_chorus_init(&d->chorus,d->history[0],d->history[1])); break;
    case 5: original_chicago_phaser_init(&d->phaser); break;
    case 10: original_chicago_ring_init(&d->ring); break;
    case 18: assert(original_chicago_mastercomp_init(&d->master)); break;
    case 22: assert(original_chicago_decimator_init(&d->decimator)); break;
    case 24: assert(original_chicago_peaking_init(&d->eq)); break;
    default: assert(0); break;
    }
    d->kernel = (original_chicago_fx2_kernel){d,direct_process,direct_parameter,
        direct_reset,noop,noop,noop_tempo,direct_activity};
}

static void direct_events(direct *d,const original_chicago_drive_event *e,uint32_t n)
{
    switch (d->type) {
    case 4: assert(original_mod_events(&d->chorus.queue,e,n)); break;
    case 5: assert(original_mod_events(&d->phaser.queue,e,n)); break;
    case 10: assert(original_mod_events(&d->ring.queue,e,n)); break;
    case 18: assert(original_chicago_mastercomp_bind_events(&d->master,e,n)); break;
    case 22: assert(original_chicago_decimator_bind_events(&d->decimator,e,n)); break;
    case 24: assert(original_chicago_peaking_bind_events(&d->eq,e,n)); break;
    default: break;
    }
}

static void direct_event(direct *d,unsigned type)
{
    switch (d->type) {
    case 4: original_chicago_chorus_direct_event(&d->chorus,type); break;
    case 5: original_chicago_phaser_direct_event(&d->phaser,type); break;
    case 10: original_chicago_ring_direct_event(&d->ring,type); break;
    case 18: original_chicago_mastercomp_direct_event(&d->master,type); break;
    case 22: original_chicago_decimator_direct_event(&d->decimator,type); break;
    case 24: original_chicago_peaking_direct_event(&d->eq,type); break;
    default: break;
    }
}

static void same_selected(const original_chicago_fx2_compact *s,const direct *d)
{
    assert(s->selected && s->selected_type == d->type);
    switch (d->type) {
    case 0: break;
    case 4: {
        original_chicago_chorus a = s->state.chorus, b = d->chorus;
        assert(a.left == s->history && a.right == s->history+1024);
        a.left = b.left = NULL; a.right = b.right = NULL;
        assert(!memcmp(&a,&b,sizeof a));
        assert(!memcmp(s->history,d->history,sizeof s->history));
        assert(s->queue.cursor == a.queue.cursor);
        break;
    }
    case 5: assert(!memcmp(&s->state.phaser,&d->phaser,sizeof d->phaser));
        assert(s->queue.cursor == d->phaser.queue.cursor); break;
    case 10: assert(!memcmp(&s->state.ring,&d->ring,sizeof d->ring));
        assert(s->queue.cursor == d->ring.queue.cursor); break;
    case 18: assert(!memcmp(&s->state.mastercomp,&d->master,sizeof d->master));
        assert(s->queue.cursor == d->master.queue.cursor); break;
    case 22: assert(!memcmp(&s->state.decimator,&d->decimator,sizeof d->decimator));
        assert(s->queue.cursor == d->decimator.queue.cursor); break;
    case 24: assert(!memcmp(&s->state.peaking,&d->eq,sizeof d->eq));
        assert(s->queue.cursor == d->eq.queue.cursor); break;
    default: assert(0); break;
    }
}

static int16_t signal_value(uint32_t x)
{
    x = (x * UINT32_C(1664525) + UINT32_C(1013904223)) >> 16;
    return x < 32768 ? (int16_t)x : (int16_t)((int32_t)x-65536);
}

static void signal(int16_t a[2][N],int16_t b[2][N],uint32_t n,unsigned seed)
{
    for (uint32_t i = 0; i < n; ++i) {
        a[0][i] = b[0][i] = signal_value(i+seed*100003u);
        a[1][i] = b[1][i] = signal_value((i+19)*17u+seed*1009u);
    }
}

static void compare_audio(int16_t a[2][N],int16_t b[2][N],uint32_t n)
{
    for (unsigned c = 0; c < 2; ++c) {
        assert(!memcmp(a[c],b[c],n*sizeof(int16_t)));
        hash_bytes(&kernel_hash,a[c],n*sizeof(int16_t));
    }
    comparisons += n;
}

static void kernels(void)
{
    static original_chicago_fx2_compact s, saved, other, other_saved;
    static direct d;
    static int16_t a[2][N],b[2][N];
    static const unsigned values[] = {0,1,63,64,126,127};
    static const uint32_t blocks[] = {0,1,31,64,65,257,1024,4096};
    static const original_chicago_drive_event events[] = {
        {0,4},{0,8},{1,4},{63,4},{64,4},{129,5},{999,4},{UINT32_MAX,4}};
    assert(!original_chicago_fx2_compact_init(NULL));
    assert(original_chicago_fx2_compact_init(&s));
    assert(original_chicago_fx2_compact_init(&other));
    original_chicago_fx2_cold_owner owner = original_chicago_fx2_compact_owner(&s);
    original_chicago_fx2_cold_owner second = original_chicago_fx2_compact_owner(&other);
    assert(second.select_cold(second.context,4));
    other_saved = other;
    for (unsigned i = 0; i < 256; ++i) {
        unsigned expected = 0;
        for (unsigned j = 0; j < 7; ++j) expected |= i == types[j];
        assert((unsigned)original_chicago_fx2_compact_supports(i) == expected);
        assert((unsigned)owner.supports(owner.context,i) == expected);
        if (!expected) {
            saved = s;
            assert(!owner.select_cold(owner.context,i));
            assert(!memcmp(&saved,&s,sizeof s));
        }
    }
    const original_chicago_drive_event bad[] = {{10,4},{9,4}};
    saved = s;
    assert(!original_chicago_fx2_compact_events(&s,NULL,1));
    assert(!original_chicago_fx2_compact_events(&s,bad,2));
    assert(!memcmp(&saved,&s,sizeof s));
    for (unsigned t = 0; t < 7; ++t) for (unsigned x = 0; x < 6; ++x)
        for (unsigned y = 0; y < 6; ++y) {
            owner.discard(owner.context);
            assert(original_chicago_fx2_compact_events(&s,events,8));
            const original_chicago_fx2_kernel *k = owner.select_cold(owner.context,types[t]);
            assert(k);
            direct_init(&d,types[t]); direct_events(&d,events,8);
            k->begin(k->context); k->reset(k->context); direct_reset(&d);
            k->parameter(k->context,2,values[x]); direct_parameter(&d,2,values[x]);
            k->parameter(k->context,3,values[y]); direct_parameter(&d,3,values[y]);
            k->tempo(k->context,4000);
            k->activity(k->context,1); direct_activity(&d,1);
            same_selected(&s,&d);
            for (unsigned j = 0; j < sizeof blocks/sizeof blocks[0]; ++j) {
                uint32_t n = blocks[j];
                signal(a,b,n,t*36+x*6+y+j);
                if (j == 3) { k->activity(k->context,0); direct_activity(&d,0); }
                if (j == 4) {
                    k->parameter(k->context,2,127-values[x]);
                    direct_parameter(&d,2,127-values[x]);
                    k->parameter(k->context,3,127-values[y]);
                    direct_parameter(&d,3,127-values[y]);
                    k->tempo(k->context,200);
                }
                if (j >= 5) {
                    unsigned event = j == 5 ? 2 : 5;
                    original_chicago_fx2_compact_direct_event(&s,event); direct_event(&d,event);
                }
                k->process(k->context,a[0],a[1],n);
                direct_process(&d,b[0],b[1],n);
                compare_audio(a,b,n); same_selected(&s,&d);
            }
            assert(!memcmp(&other,&other_saved,sizeof other));
        }
    owner.discard(owner.context);
    assert(!s.selected && !s.queue.events && !s.queue.count && !s.queue.cursor);
}

/* Observe real retire callbacks, including the union constructor's retirement.
 * The compact object is the first member, so its callback address also names
 * this surrounding test observer. No production lifecycle is replaced. */
typedef struct {
    original_chicago_fx2_compact compact;
    void (*real_retire)(void *);
    unsigned retires;
} tracked;
static void tracked_retire(void *p)
{
    tracked *s = p;
    assert(s->compact.selected);
    s->retires++;
    s->real_retire(p);
}
static void tracked_init(tracked *s)
{
    memset(s,0,sizeof *s);
    assert(original_chicago_fx2_compact_init(&s->compact));
    s->real_retire = s->compact.kernel.retire;
    s->compact.kernel.retire = tracked_retire;
}

static void wrappers(void)
{
    static tracked s;
    static direct d;
    static int16_t a[2][N],b[2][N],sa[4][N],sb[4][N];
    original_chicago_fx2_workspace wa = {sa[0],sa[1],sa[2],sa[3],N};
    original_chicago_fx2_workspace wb = {sb[0],sb[1],sb[2],sb[3],N};
    original_chicago_fx2_wrapper w,reference,before;
    tracked_init(&s);
    original_chicago_fx2_cold_owner cold = original_chicago_fx2_compact_owner(&s.compact);
    assert(original_chicago_fx2_wrapper_init_cold(&w,&cold));
    assert(!w.owner.find && !w.owner.reset_all && w.cold_policy);
    before = w;
    assert(!original_chicago_fx2_wrapper_set(&w,0,3));
    assert(!original_chicago_fx2_wrapper_set(&w,2,128));
    assert(!original_chicago_fx2_wrapper_tempo(&w,199));
    assert(!memcmp(&w,&before,sizeof w));
    unsigned selections = 0;
    static const unsigned sequence[] = {4,18,5,24,10,22,0,4,4,18,4};
    for (unsigned step = 0; step < sizeof sequence/sizeof sequence[0]; ++step) {
        unsigned type = sequence[step];
        assert(original_chicago_fx2_wrapper_set(&w,0,type));
        if (step == 8) { /* A->B->A remains a real cold A selection. */
            assert(original_chicago_fx2_wrapper_set(&w,0,5));
            assert(original_chicago_fx2_wrapper_set(&w,0,type));
        }
        assert(original_chicago_fx2_wrapper_set(&w,2,(step*13)%128));
        assert(original_chicago_fx2_wrapper_set(&w,3,127-(step*7)%128));
        assert(original_chicago_fx2_wrapper_tempo(&w,200+(int32_t)step*371));
        direct_init(&d,type);
        original_chicago_fx2_owner selected_reference = {&d,direct_find,direct_reset};
        assert(original_chicago_fx2_wrapper_init(&reference,&selected_reference));
        reference.wet_accumulator = w.wet_accumulator;
        reference.wet_target = w.wet_target;
        reference.switch_current = w.switch_current;
        reference.switch_pending = w.switch_pending;
        reference.switch_dirty = w.switch_dirty;
        assert(original_chicago_fx2_wrapper_set(&reference,0,type));
        assert(original_chicago_fx2_wrapper_set(&reference,2,w.edit1));
        assert(original_chicago_fx2_wrapper_set(&reference,3,w.edit2));
        assert(original_chicago_fx2_wrapper_tempo(&reference,w.tempo_times10));
        selections++;
        /* n0 admits/reconstructs without running any selected sample. */
        assert(original_chicago_fx2_wrapper_process(&w,&wa,a[0],a[1],0));
        assert(original_chicago_fx2_wrapper_process(&reference,&wb,b[0],b[1],0));
        assert(s.retires == selections-1 && !w.old && s.compact.selected);
        same_selected(&s.compact,&d);
        static const uint32_t blocks[] = {1,31,16,2048};
        unsigned calls = step % 2 ? 1 : 4;
        for (unsigned j = 0; j < calls; ++j) {
            uint32_t n = blocks[j];
            signal(a,b,n,step*4+j);
            assert(original_chicago_fx2_wrapper_process(&w,&wa,a[0],a[1],n));
            assert(original_chicago_fx2_wrapper_process(&reference,&wb,b[0],b[1],n));
            compare_audio(a,b,n); same_selected(&s.compact,&d);
            assert(!w.old && s.retires == selections-1);
        }
        if (step == 7) {
            /* Identical type writes alone preserve the live Chorus state. */
            assert(!w.type_dirty);
            assert(original_chicago_fx2_wrapper_set(&w,0,type));
            assert(!w.type_dirty);
        }
    }
    original_chicago_fx2_wrapper_reset(&w);
    direct_reset(&d);
    same_selected(&s.compact,&d);
    assert(s.retires == selections-1);
    cold.discard(cold.context);
    assert(s.retires == selections && !s.compact.selected);
    cold.discard(cold.context);
    assert(s.retires == selections);

    /* Cold selection keeps the consumed cursor, including duplicate offsets,
     * while direct reset and ordinary edits leave the remaining stream live. */
    static const original_chicago_drive_event events[] = {{0,4},{0,8},{64,4},{128,4}};
    assert(original_chicago_fx2_compact_events(&s.compact,events,4));
    assert(original_chicago_fx2_wrapper_init_cold(&w,&cold));
    assert(original_chicago_fx2_wrapper_set(&w,0,18));
    signal(a,b,1,1);
    assert(original_chicago_fx2_wrapper_process(&w,&wa,a[0],a[1],1));
    assert(s.compact.queue.cursor == 2);
    assert(original_chicago_fx2_wrapper_set(&w,0,4));
    assert(original_chicago_fx2_wrapper_process(&w,&wa,a[0],a[1],0));
    assert(s.compact.state.chorus.queue.cursor == 2);
    original_chicago_fx2_compact_direct_event(&s.compact,5);
    assert(s.compact.queue.cursor == 2);
    assert(original_chicago_fx2_wrapper_process(&w,&wa,a[0],a[1],65));
    assert(s.compact.queue.cursor == 3);
    cold.discard(cold.context);
}

typedef struct {
    uint32_t core[2][256],expected[2][256];
    int16_t converted[2][256],scratch[4][256];
    int16_t ref_converted[2][256],ref_scratch[4][256];
    float out[2][256];
    original_chicago_backend_workspace workspace;
} audio_workspace;

static void workspace_init(audio_workspace *w)
{
    memset(w,0,sizeof *w);
    w->workspace = (original_chicago_backend_workspace){w->core[0],w->core[1],
        w->converted[0],w->converted[1],
        {w->scratch[0],w->scratch[1],w->scratch[2],w->scratch[3]},256};
}

static void backend_audio(void)
{
    static original_chicago_backend s, saved;
    static original_chicago_fx2_compact compact, compact_saved;
    static original_chicago_engine_output reference;
    static direct d;
    static audio_workspace w;
    original_chicago_tuning tuning;
    assert(original_chicago_tuning_bind(&tuning,0,0,NULL,0x6d70));
    assert(original_chicago_fx2_compact_init(&compact));
    original_chicago_fx2_cold_owner owner = original_chicago_fx2_compact_owner(&compact);
    workspace_init(&w);
    unsigned accepted = 0, rejected = 0, mask = 0;
    for (unsigned factory = 0; factory < 81; ++factory) {
        original_chicago_program p;
        original_chicago_routing r;
        assert(original_chicago_program_init(&p,factory));
        assert(original_chicago_program_routing(&p,&r));
        saved = s; compact_saved = compact;
        int result = original_chicago_backend_init_compact(&s,factory,123,0,&tuning,&owner);
        if (!original_chicago_fx2_compact_supports(r.fx[1][0])) {
            assert(result == ORIGINAL_CHICAGO_BACKEND_INVALID);
            assert(!memcmp(&s,&saved,sizeof s));
            assert(!memcmp(&compact,&compact_saved,sizeof compact));
            rejected++;
            continue;
        }
        assert(result == OK && s.output.fx2.cold_policy && !compact.selected);
        assert(!memcmp(p.macro,s.program.macro,sizeof p.macro));
        accepted++;
        for (unsigned i = 0; i < 7; ++i) if (r.fx[1][0] == types[i]) mask |= 1u<<i;
        direct_init(&d,r.fx[1][0]); direct_reset(&d);
        original_chicago_fx2_owner selected_reference = {&d,direct_find,direct_reset};
        assert(original_chicago_engine_output_init(&reference,&r,&selected_reference));
        assert(original_chicago_fx2_wrapper_tempo(&reference.fx2,1230));
        original_chicago_output_mute(&reference.output,0);
        original_chicago_output_workspace ow = {w.ref_converted[0],w.ref_converted[1],
            w.ref_scratch[0],w.ref_scratch[1],256};
        original_chicago_fx2_workspace fw = {w.ref_scratch[0],w.ref_scratch[1],
            w.ref_scratch[2],w.ref_scratch[3],256};
        for (unsigned block = 0; block < 8; ++block) {
            uint32_t n = block % 2 ? 31 : 256;
            if (block == 2) {
                assert(original_chicago_backend_set(&s,18,.73123455f) == OK);
                assert(original_chicago_backend_set(&s,19,.18234569f) == OK);
                assert(bits(s.program.macro[18]) == bits(.73123455f));
                assert(original_chicago_engine_output_apply(&reference,&s.controls.routing,18));
                assert(original_chicago_engine_output_apply(&reference,&s.controls.routing,19));
                assert(original_chicago_backend_tempo(&s,217.5f) == OK);
                assert(original_chicago_fx2_wrapper_tempo(&reference.fx2,2175));
            }
            original_engine_event event = {0,ORIGINAL_EVENT_NOTE,48+factory%24,block == 6 ? 0u : 103u};
            int has_event = block == 0 || block == 6;
            assert(original_chicago_backend_begin(&s,&w.workspace,has_event?&event:NULL,
                (uint32_t)has_event,n) == OK);
            for (uint32_t i = 0; i < n; ++i) assert(original_chicago_backend_frame(&s) == OK);
            assert(original_chicago_engine_output_process(&reference,&ow,&fw,&s.controls.routing,
                w.core[0],w.core[1],w.expected[0],w.expected[1],n,0));
            assert(original_chicago_backend_end(&s,w.out[0],w.out[1]) == OK);
            assert(!memcmp(w.out[0],w.expected[0],n*sizeof(float)));
            assert(!memcmp(w.out[1],w.expected[1],n*sizeof(float)));
            for (uint32_t i = 0; i < n; ++i) {
                assert(isfinite(w.out[0][i]) && isfinite(w.out[1][i]));
                nonzero_frames += w.out[0][i] != 0 || w.out[1][i] != 0;
                stereo_frames += bits(w.out[0][i]) != bits(w.out[1][i]);
            }
            hash_bytes(&backend_hash,w.out[0],n*sizeof(float));
            hash_bytes(&backend_hash,w.out[1],n*sizeof(float));
            backend_frames += n;
        }
        assert(original_chicago_backend_retire(&s) == OK);
        assert(!compact.selected && !compact.queue.events);
    }
    assert(accepted == 55 && rejected == 26 && mask == 127);
    assert(nonzero_frames && stereo_frames);
}

static void backend_lifecycle(void)
{
    static original_chicago_backend a,b,saved;
    static tracked fa,fb,saved_fa;
    static audio_workspace wa,wb;
    original_chicago_tuning ta,tb;
    assert(original_chicago_tuning_bind(&ta,0,0,NULL,0x6d70)); tb = ta;
    tracked_init(&fa); tracked_init(&fb);
    original_chicago_fx2_cold_owner oa = original_chicago_fx2_compact_owner(&fa.compact);
    original_chicago_fx2_cold_owner ob = original_chicago_fx2_compact_owner(&fb.compact);
    workspace_init(&wa); workspace_init(&wb);
    assert(original_chicago_backend_init_compact(&a,0,120,0,&ta,&oa) == OK);
    assert(original_chicago_backend_init_compact(&b,0,120,0,&tb,&ob) == OK);
    assert(original_chicago_backend_init(&a,0,120,0,&ta,NULL) == ORIGINAL_CHICAGO_BACKEND_NEEDS_FX_OWNER);
    for (unsigned i = 0; i < 7; ++i) {
        float macro = (float)macro_indices[i]/11.0f;
        assert(original_chicago_backend_set(&a,17,macro) == OK);
        assert(original_chicago_backend_set(&b,17,macro) == OK);
        assert(a.controls.routing.fx[1][0] == types[i]);
        original_engine_event events[] = {
            {0,ORIGINAL_EVENT_NOTE,60,97},
            {15,ORIGINAL_EVENT_MACRO,18,bits(.8234567f)},
            {15,ORIGINAL_EVENT_MACRO,19,bits(.1234567f)},
            {30,ORIGINAL_EVENT_NOTE,60,0}};
        assert(original_chicago_backend_render(&a,&wa.workspace,events,4,wa.out[0],wa.out[1],256) == OK);
        saved = b; saved_fa = fa;
        assert(original_chicago_backend_render(&b,&wb.workspace,events,4,wb.out[0],wb.out[1],256) == OK);
        assert(!memcmp(&fa,&saved_fa,sizeof fa));
        assert(!memcmp(wa.out,wb.out,sizeof wa.out));
        assert(fa.compact.selected_type == types[i] && fb.compact.selected_type == types[i]);
        assert(!a.output.fx2.old && !b.output.fx2.old);
    }
    assert(fa.retires == 6 && fb.retires == 6);
    saved = a; saved_fa = fa;
    assert(original_chicago_backend_set(&a,17,6.0f/11.0f) == ORIGINAL_CHICAGO_BACKEND_INVALID);
    assert(original_chicago_backend_set(&a,18,NAN) == ORIGINAL_CHICAGO_BACKEND_INVALID);
    assert(original_chicago_backend_tempo(&a,INFINITY) == ORIGINAL_CHICAGO_BACKEND_INVALID);
    original_engine_event bad[] = {{0,ORIGINAL_EVENT_NOTE,62,100},
        {1,ORIGINAL_EVENT_MACRO,17,bits(6.0f/11.0f)}};
    assert(original_chicago_backend_begin(&a,&wa.workspace,bad,2,256) == ORIGINAL_CHICAGO_BACKEND_INVALID);
    assert(!memcmp(&a,&saved,sizeof a) && !memcmp(&fa,&saved_fa,sizeof fa));
    assert(original_chicago_backend_reset(&a,81) == ORIGINAL_CHICAGO_BACKEND_INVALID);
    assert(!memcmp(&a,&saved,sizeof a));
    assert(original_chicago_backend_begin(&a,&wa.workspace,NULL,0,31) == OK);
    assert(original_chicago_backend_retire(&a) == ORIGINAL_CHICAGO_BACKEND_INVALID);
    assert(original_chicago_backend_reset(&a,0) == ORIGINAL_CHICAGO_BACKEND_INVALID);
    for (unsigned i = 0; i < 31; ++i) assert(original_chicago_backend_frame(&a) == OK);
    assert(original_chicago_backend_end(&a,wa.out[0],wa.out[1]) == OK);
    assert(original_chicago_backend_reset(&a,0) == OK);
    assert(fa.retires == 7 && !fa.compact.selected && !fa.compact.queue.events);
    assert(original_chicago_backend_render(&a,&wa.workspace,NULL,0,wa.out[0],wa.out[1],31) == OK);
    assert(fa.retires == 7 && fa.compact.selected);
    assert(original_chicago_backend_retire(&a) == OK);
    assert(fa.retires == 8 && !fa.compact.selected && !a.output.fx2.active && !a.output.fx2.old);
    assert(original_chicago_backend_retire(&a) == ORIGINAL_CHICAGO_BACKEND_INVALID);
    assert(fa.retires == 8);
    assert(original_chicago_backend_retire(&b) == OK);
}

/* An ARP restart leaves its descriptor behind after the voice is silent.
 * This real note lifecycle used to keep the app's preset request pending
 * forever. Observe idle through the public API without clearing native data. */
static void backend_idle_after_restart(void)
{
    static original_chicago_backend s;
    static original_chicago_fx2_compact compact;
    static audio_workspace w;
    original_chicago_tuning tuning;
    assert(original_chicago_tuning_bind(&tuning,0,0,NULL,0x6d70));
    assert(original_chicago_fx2_compact_init(&compact));
    original_chicago_fx2_cold_owner owner = original_chicago_fx2_compact_owner(&compact);
    workspace_init(&w);
    assert(original_chicago_backend_init_compact(&s,0,120,0,&tuning,&owner) == OK);
    assert(original_chicago_backend_set(&s,17,1.0f/11.0f) == OK);
    for (unsigned b = 0; b < 14; ++b)
        assert(original_chicago_backend_render(&s,&w.workspace,NULL,0,w.out[0],w.out[1],256) == OK);
    assert(!original_chicago_backend_active(&s));
    assert(original_chicago_backend_prepare(&s) == OK);
    assert(original_chicago_backend_note(&s,60,100) == OK);
    assert(original_chicago_backend_active(&s));
    for (unsigned b = 0; b < 24; ++b)
        assert(original_chicago_backend_render(&s,&w.workspace,NULL,0,w.out[0],w.out[1],256) == OK);
    assert(original_chicago_backend_active(&s));
    assert(original_chicago_backend_prepare(&s) == OK);
    assert(original_chicago_backend_note(&s,60,0) == OK);
    for (unsigned b = 0; b < 24; ++b)
        assert(original_chicago_backend_render(&s,&w.workspace,NULL,0,w.out[0],w.out[1],256) == OK);
    assert(!s.keys.physical_count && !s.keys.effective_latch);
    assert(!s.voice.active && s.voice.vca.state == 6 && s.pending.mask);
    original_pending_note retained = s.pending;
    assert(!original_chicago_backend_active(&s));
    assert(original_chicago_backend_render(&s,&w.workspace,NULL,0,w.out[0],w.out[1],256) == OK);
    assert(!original_chicago_backend_active(&s) && !memcmp(&retained,&s.pending,sizeof retained));
    assert(original_chicago_backend_retire(&s) == OK);
}

typedef struct { original_chicago_fx2_cold_owner real; int fail; } failure_owner;
static int failure_supports(void *p,unsigned type)
{
    failure_owner *f = p;
    return f->real.supports(f->real.context,type);
}
static const original_chicago_fx2_kernel *failure_select(void *p,unsigned type)
{
    failure_owner *f = p;
    return f->fail ? NULL : f->real.select_cold(f->real.context,type);
}
static void failure_discard(void *p)
{
    failure_owner *f = p;
    f->real.discard(f->real.context);
}

static void backend_fault(void)
{
    static original_chicago_backend s;
    static original_chicago_fx2_compact compact;
    static audio_workspace w;
    original_chicago_tuning tuning;
    assert(original_chicago_tuning_bind(&tuning,0,0,NULL,0x6d70));
    assert(original_chicago_fx2_compact_init(&compact));
    failure_owner proxy = {original_chicago_fx2_compact_owner(&compact),1};
    original_chicago_fx2_cold_owner owner = {&proxy,failure_supports,failure_select,failure_discard};
    workspace_init(&w);
    for (unsigned c = 0; c < 2; ++c) for (unsigned i = 0; i < 256; ++i) w.out[c][i] = 17.0f;
    assert(original_chicago_backend_init_compact(&s,0,120,0,&tuning,&owner) == OK);
    /* A total-owner contract failure must reach the backend status. It must
     * never be reported as a successful dry/bypass or overwrite caller audio. */
    assert(original_chicago_backend_render(&s,&w.workspace,NULL,0,w.out[0],w.out[1],31)
        == ORIGINAL_CHICAGO_BACKEND_FAULT);
    assert(!s.block_open && !s.workspace);
    for (unsigned c = 0; c < 2; ++c) for (unsigned i = 0; i < 256; ++i) assert(w.out[c][i] == 17.0f);
    assert(original_chicago_backend_set(&s,18,.5f) == ORIGINAL_CHICAGO_BACKEND_FAULT);
    assert(original_chicago_backend_prepare(&s) == ORIGINAL_CHICAGO_BACKEND_FAULT);
    proxy.fail = 0;
    assert(original_chicago_backend_reset(&s,0) == OK);
    assert(original_chicago_backend_render(&s,&w.workspace,NULL,0,w.out[0],w.out[1],31) == OK);
    assert(original_chicago_backend_retire(&s) == OK);
}

int main(void)
{
    kernels(); wrappers(); backend_audio(); backend_lifecycle(); backend_idle_after_restart(); backend_fault();
    printf("PASS compact Chicago:7 direct kernels,36 control pairs each,%u exact stereo kernel frames;55 supported/26 rejected raw factories,%u exact backend output frames,%u nonzero,%u stereo;two disjoint parts,cold switches,queue/reset/retire/refusal,retained-descriptor idle;kernel=%016llx;backend=%016llx\n",
        comparisons,backend_frames,nonzero_frames,stereo_frames,
        (unsigned long long)kernel_hash,(unsigned long long)backend_hash);
    return 0;
}
