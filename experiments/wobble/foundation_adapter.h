/* SPDX-License-Identifier: GPL-3.0-only */
/* Host-only compatibility with PR212 at 6f717e216390ffca0d20be9e2204ab401ad09792.
 * Include path must point at that exact foundation's firmware/src/dsp directory.
 * This is not a firmware registry entry or a stereo integration. */
#pragma once
#ifndef X0X_SYNTH_HOST_TEST
#error "Experimental wobble compatibility adapter is host-test only"
#endif
#include "synth_engine.h"
#include "wobble.h"
#define WOBBLE_EXPERIMENT_ENGINE 0x57424c31u /* WBL1; distinct from native303=1 */
#define WOBBLE_EXPERIMENT_SCHEMA 1u
#define WOBBLE_EXPERIMENT_NPARAMS 19u
static void wobble_foundation_init(void *p) { wobble_init(p, SYNTH_SAMPLE_RATE); }
static void wobble_foundation_render(void *p, float *out, int n)
{
    /* Fixed1024-byte temporary stack. Root must qualify target stack or replace
     * this with reviewed stereo/scratch mixing before any firmware registration. */
    float right[SYNTH_MAX_BLOCK];
    if (n < 1 || n > (int)SYNTH_MAX_BLOCK) return;
    wobble_render(p, out, right, (size_t)n);
    for (int i = 0; i < n; ++i) out[i] = .5f * (out[i] + right[i]);
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
    if (parameter < 0 || parameter >= (int)WOBBLE_EXPERIMENT_NPARAMS) return;
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
