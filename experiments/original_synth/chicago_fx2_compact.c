/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_fx2_compact.h"
#include <string.h>

#define COMPACT_READY UINT32_C(0x43464331)

int original_chicago_fx2_compact_supports(unsigned type)
{
    return type == 0 || type == 3 || type == 4 || type == 5 || type == 10 ||
        type == 12 || type == 13 || type == 15 || type == 16 ||
        type == 18 || type == 22 || type == 24;
}

/* Original FX2 types restored from wip/one-build: 3 Ensemble, 12 Short Delay,
 * 13 BPM Delay, 15 Hall, 16 Gate Reverb. Each one is a mono float insert;
 * the wet difference (y - mono) is added to both channels. */
static int restored(unsigned type)
{
    return type == 3 || type == 12 || type == 13 || type == 15 || type == 16;
}

static void restored_set(original_chicago_fx2_compact *s)
{
    float bpm = (float)s->tempo_times10 * 0.1f;
    original_chicago_fx2_compact_arena *a = &s->arena;
    switch (s->selected_type) {
    case 3: chi_ens_set(&a->ensemble,s->edit1,s->edit2,44100.0f); break;
    case 12: chi_sdly_set(&a->short_delay,s->edit1,s->edit2,44100.0f); break;
    case 13: {
        /* chi_bpmdly_set_bpm() law at the wip reach CHI_BPMDLY_MAX. */
        float send = (float)s->edit1 * (1.0f / 127.0f), ms, samp;
        float max_ms = (float)(CHI_BPMDLY_MAX - 1) * 1000.0f / 44100.0f;
        if (bpm < 20.0f) bpm = 20.0f;
        if (bpm > 300.0f) bpm = 300.0f;
        ms = chi_bpmdly_beats((uint8_t)((uint32_t)s->edit2 * 12u / 127u)) * (60000.0f / bpm);
        while (ms > max_ms) ms *= 0.5f;
        samp = ms * 44.1f;
        if (samp < 1.0f) samp = 1.0f;
        if (samp > (float)(CHI_BPMDLY_MAX - 1)) samp = (float)(CHI_BPMDLY_MAX - 1);
        a->bpm_delay.delay = (uint16_t)(samp + 0.5f);
        a->bpm_delay.wet = send < 0.5f ? 0.5f : send;
        a->bpm_delay.fb = 0.35f + send * 0.5f;
        break;
    }
    case 15: chi_hall_set(&a->hall,s->edit1,s->edit2,44100.0f); break;
    case 16: chi_gaterev_set(&a->gate_reverb,s->edit1,s->edit2,44100.0f); break;
    default: break;
    }
}

static void restored_init(original_chicago_fx2_compact *s,unsigned type)
{
    original_chicago_fx2_compact_arena *a = &s->arena;
    switch (type) {
    case 3: chi_ens_init(&a->ensemble); break;
    case 12: chi_sdly_init(&a->short_delay); break;
    case 13: memset(&a->bpm_delay,0,sizeof a->bpm_delay); a->bpm_delay.delay = 512; break;
    case 15: chi_hall_init(&a->hall); break;
    case 16: chi_gaterev_init(&a->gate_reverb); break;
    default: break;
    }
}

/* wip chi_bpmdly_process with the ring written once per two samples
 * (pair mean) and read with linear interpolation between ring slots. */
static void bpm_process(original_chicago_fx2_compact_bpm *d,float *buf,int n)
{
    const uint32_t N = ORIGINAL_CHICAGO_FX2_COMPACT_BPM_SLOTS;
    float wet = d->wet, dry_g, fb = d->fb;
    uint32_t delay = d->delay;
    if (wet <= 0.0f) return;
    dry_g = 1.0f - wet;
    if (delay < 2u) delay = 2u;
    if (delay > 2u * N - 4u) delay = 2u * N - 4u;
    for (int k = 0; k < n; k++) {
        float x = buf[k], tap, delayed;
        /* Sample time t = 2*w + phase; read t - delay in half-rate slots. */
        uint32_t back = delay - d->phase; /* samples behind slot w */
        uint32_t whole = (back + 1u) >> 1, frac2 = (back + 1u) & 1u;
        uint32_t i0 = (d->w + N - whole) % N, i1 = (i0 + 1u) % N;
        float a = (float)d->ring[i0] * (1.0f / 32767.0f);
        float b = i1 == d->w ? d->last : (float)d->ring[i1] * (1.0f / 32767.0f);
        delayed = frac2 ? a + (b - a) * 0.5f : a;
        tap = x + fb * delayed;
        if (tap > 1.0f) tap = 1.0f; else if (tap < -1.0f) tap = -1.0f;
        if (d->phase) {
            d->last = (d->pair + tap) * 0.5f;
            d->ring[d->w] = chi_bpmdly_q15(d->last);
            d->w = (uint16_t)((d->w + 1u) % N);
            d->phase = 0;
        } else {
            d->pair = tap;
            d->phase = 1;
        }
        buf[k] = dry_g * x + wet * delayed;
    }
}

static void restored_process(original_chicago_fx2_compact *s,int16_t *left,int16_t *right,uint32_t n)
{
    float mono[64];
    original_chicago_fx2_compact_arena *a = &s->arena;
    while (n) {
        uint32_t m = n < 64u ? n : 64u;
        for (uint32_t i = 0; i < m; ++i)
            mono[i] = ((float)left[i] + (float)right[i]) * (0.5f / 32768.0f);
        float dry[64];
        memcpy(dry,mono,m * sizeof mono[0]);
        switch (s->selected_type) {
        case 3: chi_ens_process(&a->ensemble,mono,(int)m); break;
        case 12: chi_sdly_process(&a->short_delay,mono,(int)m); break;
        case 13: bpm_process(&a->bpm_delay,mono,(int)m); break;
        case 15: chi_hall_process(&a->hall,mono,(int)m); break;
        case 16: chi_gaterev_process(&a->gate_reverb,mono,(int)m); break;
        default: break;
        }
        for (uint32_t i = 0; i < m; ++i) {
            float d = (mono[i] - dry[i]) * 32768.0f;
            float l = (float)left[i] + d, r = (float)right[i] + d;
            left[i] = (int16_t)(l > 32767.0f ? 32767 : l < -32768.0f ? -32768 : (int32_t)l);
            right[i] = (int16_t)(r > 32767.0f ? 32767 : r < -32768.0f ? -32768 : (int32_t)r);
        }
        left += m; right += m; n -= m;
    }
}

static int live(const original_chicago_fx2_compact *s)
{
    return s && s->ready == COMPACT_READY;
}

static void queue_to_selected(original_chicago_fx2_compact *s)
{
    const original_mod_queue *q = &s->queue;
    original_chicago_dyn_queue d = {q->events,q->count,q->cursor};
    switch (s->selected_type) {
    case 4: s->state.chorus.queue = *q; break;
    case 5: s->state.phaser.queue = *q; break;
    case 10: s->state.ring.queue = *q; break;
    case 18: s->state.mastercomp.queue = d; break;
    case 22: s->state.decimator.queue = d; break;
    case 24: s->state.peaking.queue = d; break;
    default: break;
    }
}

static void queue_from_selected(original_chicago_fx2_compact *s)
{
    switch (s->selected_type) {
    case 4: s->queue.cursor = s->state.chorus.queue.cursor; break;
    case 5: s->queue.cursor = s->state.phaser.queue.cursor; break;
    case 10: s->queue.cursor = s->state.ring.queue.cursor; break;
    case 18: s->queue.cursor = s->state.mastercomp.queue.cursor; break;
    case 22: s->queue.cursor = s->state.decimator.queue.cursor; break;
    case 24: s->queue.cursor = s->state.peaking.queue.cursor; break;
    default: break;
    }
}

static void parameter(void *p,unsigned id,unsigned value)
{
    original_chicago_fx2_compact *s = p;
    if (restored(s->selected_type)) {
        if (value > 127u) return;
        if (id == 2u) s->edit1 = (uint8_t)value;
        else if (id == 3u) s->edit2 = (uint8_t)value;
        else return;
        restored_set(s);
        return;
    }
    switch (s->selected_type) {
    case 0: (void)original_chicago_bypass_parameter(id,(int32_t)value); break;
    case 4: (void)original_chicago_chorus_set(&s->state.chorus,id,value); break;
    case 5: (void)original_chicago_phaser_set(&s->state.phaser,id,value); break;
    case 10: (void)original_chicago_ring_set(&s->state.ring,id,value); break;
    case 18: (void)original_chicago_mastercomp_set(&s->state.mastercomp,id-2,(int32_t)value); break;
    case 22: (void)original_chicago_decimator_set(&s->state.decimator,id-2,(int32_t)value); break;
    case 24: (void)original_chicago_peaking_set(&s->state.peaking,id-2,value); break;
    default: break;
    }
}

static void reset(void *p)
{
    original_chicago_fx2_compact *s = p;
    switch (s->selected_type) {
    case 4: original_chicago_chorus_reset(&s->state.chorus); break;
    case 5: original_chicago_phaser_reset(&s->state.phaser); break;
    case 10: original_chicago_ring_reset(&s->state.ring); break;
    case 18: original_chicago_mastercomp_reset(&s->state.mastercomp); break;
    case 22: original_chicago_decimator_reset(&s->state.decimator); break;
    case 24: original_chicago_peaking_reset(&s->state.peaking); break;
    default:
        if (restored(s->selected_type)) { restored_init(s,s->selected_type); restored_set(s); }
        break;
    }
}

static void process(void *p,int16_t *left,int16_t *right,uint32_t n)
{
    original_chicago_fx2_compact *s = p;
    switch (s->selected_type) {
    case 0: (void)original_chicago_bypass_process(left,right,n); break;
    case 4: (void)original_chicago_chorus_process(&s->state.chorus,left,right,n); break;
    case 5: (void)original_chicago_phaser_process(&s->state.phaser,left,right,n); break;
    case 10: (void)original_chicago_ring_process(&s->state.ring,left,right,n); break;
    case 18: (void)original_chicago_mastercomp_process(&s->state.mastercomp,left,right,n); break;
    case 22: (void)original_chicago_decimator_process(&s->state.decimator,left,right,n); break;
    case 24: (void)original_chicago_peaking_process(&s->state.peaking,left,right,n); break;
    default: if (restored(s->selected_type)) restored_process(s,left,right,n); break;
    }
    queue_from_selected(s);
}

static void begin(void *p) { (void)p; }
static void tempo(void *p,int32_t value)
{
    original_chicago_fx2_compact *s = p;
    if (value < 200 || value > 4000) return;
    s->tempo_times10 = value;
    if (s->selected_type == 13) restored_set(s); /* live-tempo BPM Delay (#677) */
}

static int advance_zero(void *p,uint32_t n,int16_t constant[2])
{
    original_chicago_fx2_compact *s=p;
    if(!live(s)||!s->selected||!constant)return 0;
    int16_t value[2]={0,0};
    int ok=0;
    switch(s->selected_type){
    case 0: ok=1; break;
    case 4: ok=original_chicago_chorus_advance_zero(&s->state.chorus,n); break;
    case 5: ok=original_chicago_phaser_advance_zero(&s->state.phaser,n); break;
    case 10: ok=original_chicago_ring_advance_zero(&s->state.ring,n); break;
    case 18: ok=original_chicago_mastercomp_advance_zero(&s->state.mastercomp,n); break;
    case 22: ok=original_chicago_decimator_advance_zero(&s->state.decimator,n); break;
    case 24: ok=original_chicago_peaking_advance_zero(&s->state.peaking,n,value); break;
    default: break;
    }
    if(ok){queue_from_selected(s);constant[0]=value[0];constant[1]=value[1];}
    return ok;
}

static void retire(void *p)
{
    original_chicago_fx2_compact *s = p;
    s->selected = 0;
}

static void activity(void *p,unsigned value)
{
    original_chicago_fx2_compact *s = p;
    switch (s->selected_type) {
    case 4: s->state.chorus.activity = (uint8_t)value; break;
    case 5: s->state.phaser.activity = (uint8_t)value; break;
    case 10: s->state.ring.activity = (uint8_t)value; break;
    case 18: original_chicago_mastercomp_activity(&s->state.mastercomp,(uint8_t)value); break;
    case 24: original_chicago_peaking_activity(&s->state.peaking,(uint8_t)value); break;
    default: break;
    }
}

static int supports(void *p,unsigned type)
{
    return live(p) && original_chicago_fx2_compact_supports(type);
}

static const original_chicago_fx2_kernel *select_cold(void *p,unsigned type)
{
    original_chicago_fx2_compact *s = p;
    if (!supports(s,type)) return NULL;
    if (s->selected) s->kernel.retire(s->kernel.context);
    /* Constructor state matters: Master reset retains its highpass history.
     * A reset of reused union bytes alone is not a valid new object. */
    memset(&s->state,0,sizeof s->state);
    switch (type) {
    case 4: (void)original_chicago_chorus_init(&s->state.chorus,s->arena.history,s->arena.history+1024); break;
    case 5: original_chicago_phaser_init(&s->state.phaser); break;
    case 10: original_chicago_ring_init(&s->state.ring); break;
    case 18: (void)original_chicago_mastercomp_init(&s->state.mastercomp); break;
    case 22: (void)original_chicago_decimator_init(&s->state.decimator); break;
    case 24: (void)original_chicago_peaking_init(&s->state.peaking); break;
    default: break;
    }
    if (restored(type)) {
        s->edit1 = s->edit2 = 64;
        restored_init(s,type);
    }
    s->selected_type = (uint8_t)type;
    if (restored(type)) restored_set(s);
    s->selected = 1;
    queue_to_selected(s);
    return &s->kernel;
}

static void discard(void *p)
{
    original_chicago_fx2_compact *s = p;
    if (!live(s)) return;
    if (s->selected) s->kernel.retire(s->kernel.context);
    memset(&s->state,0,sizeof s->state);
    memset(&s->queue,0,sizeof s->queue);
    s->selected_type = 0;
}

int original_chicago_fx2_compact_init(original_chicago_fx2_compact *s)
{
    if (!s) return 0;
    memset(s,0,sizeof *s);
    s->kernel = (original_chicago_fx2_kernel){s,process,parameter,reset,begin,retire,tempo,activity};
    s->tempo_times10 = 1200;
    s->ready = COMPACT_READY;
    return 1;
}

original_chicago_fx2_cold_owner original_chicago_fx2_compact_owner(original_chicago_fx2_compact *s)
{
    original_chicago_fx2_cold_owner owner = {s,supports,select_cold,discard,advance_zero};
    return owner;
}

int original_chicago_fx2_compact_events(original_chicago_fx2_compact *s,
    const original_chicago_drive_event *events,uint32_t n)
{
    if (!live(s) || (n && !events)) return 0;
    for (uint32_t i = 1; i < n; ++i)
        if (events[i-1].offset > events[i].offset) return 0;
    s->queue = (original_mod_queue){events,n,0};
    if (s->selected) queue_to_selected(s);
    return 1;
}

void original_chicago_fx2_compact_direct_event(original_chicago_fx2_compact *s,unsigned type)
{
    if (!live(s) || !s->selected) return;
    switch (s->selected_type) {
    case 4: original_chicago_chorus_direct_event(&s->state.chorus,type); break;
    case 5: original_chicago_phaser_direct_event(&s->state.phaser,type); break;
    case 10: original_chicago_ring_direct_event(&s->state.ring,type); break;
    case 18: original_chicago_mastercomp_direct_event(&s->state.mastercomp,type); break;
    case 22: original_chicago_decimator_direct_event(&s->state.decimator,type); break;
    case 24: original_chicago_peaking_direct_event(&s->state.peaking,type); break;
    default: break;
    }
}
