/* SPDX-License-Identifier: GPL-3.0-only */
/* Product Miami / WBL1 backend (also host tests). */
#pragma once
#include "wobble_config.h"
#if !X0X_WOBBLE && !defined(X0X_SYNTH_HOST_TEST)
#error "Wobble backend requires X0X_WOBBLE=1 or host synth tests"
#endif
#include "synth_engine.h"
#include "../../../experiments/wobble/wobble.h"
#define WOBBLE_EXPERIMENT_ENGINE 0x57424c31u /* WBL1; distinct from native303=1 */
#define WOBBLE_EXPERIMENT_SCHEMA 1u
static void wobble_foundation_init(void *p) { wobble_init(p, SYNTH_SAMPLE_RATE); }
static void wobble_foundation_render(void *p, float *out, int n)
{
    /* Bounded 16-frame mono compatibility scratch, not the stereo mixer path. */
    float right[16];
    if (n < 1 || n > (int)SYNTH_MAX_BLOCK) return;
    for (int pos = 0; pos < n; pos += 16) {
        int count = n - pos < 16 ? n - pos : 16;
        wobble_render(p, out + pos, right, (size_t)count);
        for (int i = 0; i < count; ++i) out[pos+i] = .5f * (out[pos+i] + right[i]);
    }
}
static void wobble_foundation_note(void *p, int note, int accent, int slide, int velocity)
{
    /* Accent/slide do not import303 circuit semantics. Explicit velocity and
     * existing glide control determine this original instrument's response. */
    (void)accent; (void)slide;
    if (note < 0) note = 0;
    if (note > 127) note = 127;
    if (velocity < 0) velocity = 0;
    if (velocity > 127) velocity = 127;
    wobble_note_on(p, (uint8_t)note, (uint8_t)velocity);
}
static void wobble_foundation_off(void *p) { wobble_all_notes_off(p); }
static void wobble_foundation_panic(void *p) { wobble_reset(p); }
static void wobble_foundation_set(void *p, int parameter, int value)
{
    if (parameter < 0 || parameter >= (int)W_PARAMETER_COUNT) return;
    if (value < 0) value = 0;
    if (value > 127) value = 127;
    wobble_set(p, (unsigned)parameter, (float)value/127.0f);
}
static void wobble_foundation_transport(void *p, int running) { (void)p; (void)running; }
static int wobble_foundation_active(const void *p) { return ((const wobble_t *)p)->gate; }
static int wobble_foundation_idle(const void *p) { return !((const wobble_t *)p)->active; }
/* No alternate lite-quality implementation is claimed. */
static void wobble_foundation_lite(void *p, int lite) { (void)p; (void)lite; }
static const synth_ops_t wobble_foundation_ops = {
    WOBBLE_EXPERIMENT_ENGINE, WOBBLE_EXPERIMENT_SCHEMA, WOBBLE_EXPERIMENT_NPARAMS,
    wobble_foundation_init, wobble_foundation_render, wobble_foundation_note,
    wobble_foundation_off, wobble_foundation_panic, wobble_foundation_set,
    wobble_foundation_transport, wobble_foundation_active, wobble_foundation_idle,
    wobble_foundation_lite
};
static inline synth_binding_t wobble_foundation_binding(wobble_t *context)
{ synth_binding_t b = { &wobble_foundation_ops, context }; return b; }

/* Distinct caller-owned buffers; preserves original side signal. */
static inline int wobble_backend_stereo(synth_slot_t *s, float *l, float *r, int n)
{
    if (!s || !l || !r || l == r || n < 1 || n > (int)SYNTH_MAX_BLOCK ||
        s->active.ops != &wobble_foundation_ops) return 0;
    wobble_render(s->active.context, l, r, (size_t)n); return 1;
}
/* Version1 retained bank is exactly sixteen 7-bit control bytes, no raw floats. */
static inline int wobble_bank_load(wobble_t *s, uint32_t id, unsigned schema,
                                   const uint8_t *bank, size_t n)
{
    if (!s || !bank || id != WOBBLE_EXPERIMENT_ENGINE || schema != 1 || n != 16) return 0;
    for (unsigned i=0; i<16; ++i) if (bank[i]>127) return 0;
    for (unsigned i=0; i<16; ++i) wobble_foundation_set(s,(int)i,bank[i]);
    return 1;
}
static inline void wobble_bank_save(const wobble_t *s, uint8_t bank[16])
{ for (unsigned i=0; i<16; ++i) bank[i]=(uint8_t)(s->p[i]*127.0f+.5f); }
