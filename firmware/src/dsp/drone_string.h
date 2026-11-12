/* SPDX-License-Identifier: MIT
 * Drone String Machine: a bounded, divide-down registration oscillator.
 * Inspired by the StringSynthOscillator in Mutable Instruments Plaits:
 * Copyright (c) 2016 Emilie Gillet; MIT license.
 * https://github.com/pichenettes/eurorack/blob/master/plaits/dsp/oscillator/string_synth_oscillator.h
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Independent freestanding C implementation: three octave registrations of
 * PolyBLEP saws from one phase, warm one-pole damping, slow flutter and a
 * bounded four-voice allocator. No heap, delay line, libm or render-time setup.
 * The host and the SMK-37 use the same sample rate and arithmetic.
 */
#pragma once
#include <stdint.h>
#include "fastmath.h"

#define DRONE_STRING_VOICES 4u
#define DRONE_STRING_PARAMETERS 8u
#define DRONE_STRING_SAMPLE_RATE 44100.0f

enum { DRONE_ATTACK = 1, DRONE_HELD = 2, DRONE_RELEASE = 3 };

typedef struct {
    float phase, step, envelope, lowpass, velocity, flutter;
    uint32_t age;
    uint8_t key, stage, down;
} drone_string_voice;

typedef struct {
    drone_string_voice voice[DRONE_STRING_VOICES];
    float macro[DRONE_STRING_PARAMETERS];
    float attack_step, release_step, tone;
    float registration[3], output;
    uint32_t serial;
    uint8_t damper;
} drone_string_t;

static inline float drone_string_step(unsigned key, float bend)
{
    float semitone = (float)key - 69.0f + (bend - 0.5f) * 4.0f;
    float frequency = 440.0f * fm_exp2f(semitone * (1.0f / 12.0f));
    return fm_clampf(frequency * (1.0f / DRONE_STRING_SAMPLE_RATE), 0.00005f, 0.42f);
}

static inline void drone_string_refresh(drone_string_t *d)
{
    float reg = d->macro[0], bright = d->macro[1];
    float attack = d->macro[3], release = d->macro[4];
    d->registration[0] = 0.74f - reg * 0.17f;
    d->registration[1] = 0.22f + reg * 0.02f;
    d->registration[2] = 0.04f + reg * 0.15f;
    d->tone = 0.012f + bright * bright * 0.48f;
    d->attack_step = 1.0f / ((0.004f + attack * attack * 2.0f) * DRONE_STRING_SAMPLE_RATE);
    d->release_step = 1.0f / ((0.08f + release * release * 4.0f) * DRONE_STRING_SAMPLE_RATE);
    d->output = 0.24f * d->macro[5];
}

static inline void drone_string_init(drone_string_t *d)
{
    __builtin_memset(d, 0, sizeof *d);
    d->macro[0] = 0.45f;
    d->macro[1] = 0.52f;
    d->macro[2] = 0.40f;
    d->macro[3] = 0.18f;
    d->macro[4] = 0.65f;
    d->macro[5] = 0.75f;
    d->macro[6] = 0.5f;
    drone_string_refresh(d);
}

static inline int drone_string_set(drone_string_t *d, unsigned parameter, float value)
{
    if (!d || parameter >= DRONE_STRING_PARAMETERS || !(value >= 0.0f && value <= 1.0f))
        return 0;
    if (parameter == 6u && d->macro[6] != value) {
        for (unsigned v = 0; v < DRONE_STRING_VOICES; ++v)
            if (d->voice[v].stage)
                d->voice[v].step = drone_string_step(d->voice[v].key, value);
    }
    if (parameter == 7u) {
        uint8_t down = (uint8_t)(value >= 0.5f);
        if (d->damper && !down)
            for (unsigned v = 0; v < DRONE_STRING_VOICES; ++v)
                if (d->voice[v].stage && !d->voice[v].down)
                    d->voice[v].stage = DRONE_RELEASE;
        d->damper = down;
    }
    d->macro[parameter] = value;
    if (parameter < 6u) drone_string_refresh(d);
    return 1;
}

static inline int drone_string_note(drone_string_t *d, unsigned key, unsigned velocity)
{
    if (!d || key > 127u || velocity > 127u) return 0;
    if (!velocity) {
        for (unsigned i = 0; i < DRONE_STRING_VOICES; ++i) {
            drone_string_voice *v = &d->voice[i];
            if (v->stage && v->key == key && v->down) {
                v->down = 0;
                if (!d->damper) v->stage = DRONE_RELEASE;
            }
        }
        return 1;
    }
    drone_string_voice *pick = 0;
    for (unsigned i = 0; i < DRONE_STRING_VOICES; ++i) {
        drone_string_voice *v = &d->voice[i];
        if (v->stage && v->key == key) { pick = v; break; }
        if (!v->stage) { pick = v; break; }
    }
    if (!pick) {
        pick = &d->voice[0];
        for (unsigned i = 1; i < DRONE_STRING_VOICES; ++i) {
            drone_string_voice *v = &d->voice[i];
            if ((v->stage == DRONE_RELEASE && pick->stage != DRONE_RELEASE) ||
                (v->stage == pick->stage && v->envelope < pick->envelope) ||
                (v->stage == pick->stage && v->envelope == pick->envelope && v->age < pick->age))
                pick = v;
        }
    }
    if (!pick->stage || pick->key != key) {
        pick->phase = 0.0f; pick->envelope = 0.0f; pick->lowpass = 0.0f;
        pick->flutter = (float)(key % 7u) * (1.0f / 7.0f);
    }
    pick->key = (uint8_t)key; pick->down = 1u;
    pick->step = drone_string_step(key, d->macro[6]);
    pick->velocity = (float)velocity * (1.0f / 127.0f);
    pick->age = ++d->serial;
    pick->stage = DRONE_ATTACK;
    return 1;
}

static inline void drone_string_all_off(drone_string_t *d)
{
    for (unsigned i = 0; i < DRONE_STRING_VOICES; ++i) {
        d->voice[i].stage = 0;
        d->voice[i].down = 0;
        d->voice[i].envelope = 0.0f;
        d->voice[i].lowpass = 0.0f;
    }
    d->damper = 0;
    d->macro[7] = 0.0f;
}

static inline int drone_string_active(const drone_string_t *d)
{
    for (unsigned i = 0; i < DRONE_STRING_VOICES; ++i)
        if (d->voice[i].stage) return 1;
    return 0;
}

/* PolyBLEP for a discontinuity at phase zero. Uses two tiny edge polynomials,
 * one shared phase counter per voice and no wavetable/heap. */
static inline float drone_string_blep(float t, float dt)
{
    if (t < dt) {
        t /= dt;
        return (t + t) - t * t - 1.0f;
    }
    if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + (t + t) + 1.0f;
    }
    return 0.0f;
}

static inline float drone_string_saw(float phase, float step)
{
    return (phase + phase - 1.0f) - drone_string_blep(phase, step);
}

/* Render into the existing stereo part buffers, with no new buffering or latency.
 * A fully idle voice has no inner loop cost. Max block 256, as on the audio ISR. */
static inline int drone_string_render(drone_string_t *d, float *left, float *right, unsigned frames)
{
    if (!d || !left || !right || frames > 256u) return 0;
    static const float pan_l[DRONE_STRING_VOICES] = { 0.61f, 0.44f, 0.56f, 0.39f };
    static const float pan_r[DRONE_STRING_VOICES] = { 0.39f, 0.56f, 0.44f, 0.61f };
    for (unsigned j = 0; j < DRONE_STRING_VOICES; ++j) {
        drone_string_voice *v = &d->voice[j];
        if (!v->stage) continue;
        float phase = v->phase, env = v->envelope, low = v->lowpass, flutter = v->flutter;
        float base_step = v->step, g = d->output * v->velocity;
        float gl = g * pan_l[j], gr = g * pan_r[j];
        for (unsigned i = 0; i < frames; ++i) {
            float delta = base_step;
            if (d->macro[2] > 0.001f) {
                flutter += 0.000009f + (float)j * 0.0000011f;
                if (flutter >= 1.0f) flutter -= 1.0f;
                float triangle = flutter < 0.5f ? (flutter * 4.0f - 1.0f) : (3.0f - flutter * 4.0f);
                delta *= 1.0f + triangle * (0.0035f * d->macro[2]);
            }
            phase += delta;
            if (phase >= 1.0f) phase -= 1.0f;
            float wave = d->registration[0] * drone_string_saw(phase, delta);
            if (delta < 0.22f) {
                float p2 = phase * 2.0f;
                if (p2 >= 1.0f) p2 -= 1.0f;
                wave += d->registration[1] * drone_string_saw(p2, delta * 2.0f);
                if (delta < 0.11f) {
                    float p4 = phase * 4.0f;
                    p4 -= (float)(unsigned)p4;
                    wave += d->registration[2] * drone_string_saw(p4, delta * 4.0f);
                }
            }
            low += d->tone * (wave - low);
            if (v->stage == DRONE_ATTACK) {
                env += d->attack_step;
                if (env >= 1.0f) { env = 1.0f; v->stage = DRONE_HELD; }
            } else if (v->stage == DRONE_RELEASE) {
                env -= d->release_step;
                if (env <= 0.0f) { env = 0.0f; v->stage = 0; }
            }
            float sample = low * env;
            left[i] += sample * gl;
            right[i] += sample * gr;
            if (!v->stage) break;
        }
        v->phase = phase;
        v->envelope = env;
        v->lowpass = fm_flush(low);
        v->flutter = flutter;
    }
    return 1;
}
