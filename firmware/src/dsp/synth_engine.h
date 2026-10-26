/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "bass303.h"
#include "wobble_config.h"
#include <stddef.h>

#define SYNTH_ENGINE_303 1u
#define SYNTH_SCHEMA_303 1u
#define SYNTH_SAMPLE_RATE 44100u
#define SYNTH_MAX_BLOCK 256u
/* Caller-owned contexts and descriptors must outlive the slot. No allocation,
 * filesystem, UI or project-memory access is permitted in backend callbacks. */
typedef struct {
    uint32_t id;
    uint16_t schema, nparams;
    void (*init)(void *);
    void (*render)(void *, float *, int);
    void (*note)(void *, int, int, int, int);
    void (*off)(void *);
    void (*panic)(void *);
    void (*set)(void *, int, int);
    void (*transport)(void *, int);
    int (*active)(const void *);
    int (*idle)(const void *);
    void (*lite)(void *, int);
} synth_ops_t;
typedef struct { const synth_ops_t *ops; void *context; } synth_binding_t;
typedef struct { uint32_t engine; uint16_t schema, param; } synth_param_key_t;
typedef struct {
    synth_binding_t active, pending;
    volatile uint8_t requested, acknowledged;
    uint8_t running, lite;
} synth_slot_t;
/* Same-core main-loop producer / audio ISR consumer only. Publishing a single
 * byte occurs last; pending fields cannot be reused until acknowledgement.
 * Compiler fences suffice on this single-core interrupt contract. Other thread
 * models must supply a synchronized external queue (host tests are sequential). */
#define SYNTH_FENCE() __asm__ volatile("" ::: "memory")
static inline int synth_valid(synth_binding_t b)
{
    return b.context && b.ops && b.ops->id && b.ops->schema && b.ops->init &&
        b.ops->render && b.ops->note && b.ops->off && b.ops->panic && b.ops->set &&
        b.ops->transport && b.ops->active && b.ops->idle && b.ops->lite;
}
static inline int synth_init(synth_slot_t *s, synth_binding_t b)
{
    if (!s || !synth_valid(b)) return 0;
    s->active = b; s->pending = b; s->requested = s->acknowledged = 0;
    s->running = s->lite = 0; b.ops->init(b.context); return 1;
}
/* Alternate descriptors are test-only until native fit, lifecycle and identity
 * qualification. There is deliberately no public engine registry/menu. */
static inline int synth_request(synth_slot_t *s, synth_binding_t b, uint8_t *ticket)
{
    if (!synth_valid(b) || s->requested != s->acknowledged) return 0;
#ifndef X0X_SYNTH_HOST_TEST
    if ((b.ops->id != SYNTH_ENGINE_303 || b.ops->schema != SYNTH_SCHEMA_303)
#if X0X_DEV_WOBBLE
        && (b.ops->id != 0x57424c31u || b.ops->schema != 1 || b.ops->nparams != 16)
#endif
       ) return 0;
#endif
    SYNTH_FENCE(); s->pending = b; SYNTH_FENCE();
    s->requested = (uint8_t)(s->requested + 1u);
    if (ticket) *ticket = s->requested;
    return 1;
}
/* Invoke only at the audio command boundary, before notes/locks/render. New
 * context is already initialized; panic clears notes but retains its settings. */
static inline int synth_boundary(synth_slot_t *s)
{
    uint8_t ticket = s->requested;
    if (ticket == s->acknowledged) return 0;
    SYNTH_FENCE();
    s->active.ops->panic(s->active.context);
    if (s->active.context != s->pending.context || s->active.ops != s->pending.ops)
        s->pending.ops->panic(s->pending.context);
    s->active = s->pending;
    s->active.ops->lite(s->active.context, s->lite);
    s->active.ops->transport(s->active.context, s->running);
    SYNTH_FENCE(); s->acknowledged = ticket; return 1;
}
static inline int synth_set(synth_slot_t *s, synth_param_key_t k, int value)
{
    if (k.engine != s->active.ops->id || k.schema != s->active.ops->schema ||
        k.param >= s->active.ops->nparams) return 0;
    s->active.ops->set(s->active.context, k.param, value); return 1;
}
static inline void synth_render(synth_slot_t *s, float *out, int n)
{ s->active.ops->render(s->active.context, out, n); }
static inline void synth_note(synth_slot_t *s, int n, int a, int sl, int v)
{ s->active.ops->note(s->active.context, n, a, sl, v); }
static inline void synth_off(synth_slot_t *s) { s->active.ops->off(s->active.context); }
static inline void synth_transport(synth_slot_t *s, int running)
{
    s->running = !!running;
    if (!running) s->active.ops->panic(s->active.context);
    s->active.ops->transport(s->active.context, s->running);
}
static inline void synth_lite(synth_slot_t *s, int lite)
{ s->lite = !!lite; s->active.ops->lite(s->active.context, lite); }
static inline int synth_active(const synth_slot_t *s)
{ return s->active.ops ? s->active.ops->active(s->active.context) : 0; }
static inline int synth_idle(const synth_slot_t *s)
{ return s->active.ops ? s->active.ops->idle(s->active.context) : 1; }

static void synth303_init(void *p) { bass303_init(p); }
static void synth303_render(void *p, float *o, int n) { bass303_render(p, o, n); }
static void synth303_note(void *p, int n, int a, int sl, int v) { bass303_note_on_velocity(p,n,a,sl,v); }
static void synth303_off(void *p) { bass303_note_off(p); }
static void synth303_panic(void *p) { bass303_all_off(p); }
static void synth303_set(void *p, int i, int v) { bass303_set(p,i,v); }
static void synth303_transport(void *p, int run) { (void)p; (void)run; }
static int synth303_active(const void *p) { return ((const bass303_t *)p)->gate; }
static int synth303_idle(const void *p) { return ((const bass303_t *)p)->idle; }
static void synth303_lite(void *p, int on) { bass303_set_lite(p,on); }
static const synth_ops_t synth303_ops = {
    SYNTH_ENGINE_303, SYNTH_SCHEMA_303, BASS303_NPARAMS,
    synth303_init, synth303_render, synth303_note, synth303_off, synth303_panic,
    synth303_set, synth303_transport, synth303_active, synth303_idle, synth303_lite
};
static inline synth_binding_t synth303_binding(bass303_t *context)
{ synth_binding_t b = { &synth303_ops, context }; return b; }
static inline synth_param_key_t synth303_key(unsigned param)
{ synth_param_key_t k = { SYNTH_ENGINE_303, SYNTH_SCHEMA_303, (uint16_t)param }; return k; }
