/* SPDX-License-Identifier: GPL-3.0-only */
/* Chicago Arp Switch / Mode / Range / Speed (plugin ids 13–16).
 *
 * Stored in sound_t after the bass303 DSP pots (CHI_ARP_*). Not consumed by
 * bass303_set: the live keyboard path owns note generation when Switch is On.
 * Pattern playback (seq sink) is unchanged. Factory pots are 0..255; UI pots
 * are discrete 0..n-1 with the Chicago labels.
 *
 * Note pick + rate tables reuse clip-mode arp (smk_clip_arp_share.c ck_arp_pick /
 * CK_RATE_T; one copy for engine.o + x0x.o). Clip MIDI→Loopy (main_fm1.c ck_arp*)
 * stays separate I/O; this header is the thin Chicago surface + sample-clock
 * runtime for native303. */
#pragma once
#include <stdint.h>
#include "../dsp/x0x_param.h"
#include "smk_clip_keys.h"

/* Indices sit after last bass303 pot (ARELEASE=22); BASS303_NPARAMS is 23.
 * Do not retarget AmpMod/Gnaw/Bite/ADSR. */
enum {
    CHI_ARP_SWITCH = 23,
    CHI_ARP_MODE,
    CHI_ARP_RANGE,
    CHI_ARP_SPEED,
    CHI_ARP_NPARAMS_END /* engine_nparams(T_303) == 27 */
};

enum { CHI_ARP_OFF, CHI_ARP_ON };
enum { CHI_ARP_DOWN, CHI_ARP_UP, CHI_ARP_UPDN, CHI_ARP_NMODE };
enum { CHI_ARP_OCT1, CHI_ARP_OCT2, CHI_ARP_FULL, CHI_ARP_NRANGE };
enum {
    CHI_ARP_SPD_1_2,
    CHI_ARP_SPD_3_8,
    CHI_ARP_SPD_1_3,
    CHI_ARP_SPD_1_4,
    CHI_ARP_SPD_3_16,
    CHI_ARP_SPD_1_6,
    CHI_ARP_SPD_1_8,
    CHI_ARP_SPD_1_12,
    CHI_ARP_SPD_1_16,
    CHI_ARP_SPD_1_32,
    CHI_ARP_NSPEED
};

#define CHI_ARP_MAXN 8u /* held notes in one live chord (product path) */

/* Chicago Speed → clip CK_RATE_T index (subset; skips 1/16D and 1/16T).
 * Tables live in chicago_param_tables.c (one copy for engine.o + x0x.o). */
extern const uint8_t CHI_ARP_RATE_IX[CHI_ARP_NSPEED];

extern const char *const CHI_ARP_SWITCH_N[];
extern const char *const CHI_ARP_MODE_N[];
extern const char *const CHI_ARP_RANGE_N[];
extern const char *const CHI_ARP_SPEED_N[];

extern const x0x_param_t CHI_ARP_PARAMS[4];

/* Octaves sounded: 1 / 2 / Full. Full is provisional 3 octaves until AU characterization. */
static inline uint32_t chi_arp_octaves(uint32_t range)
{
    if (range >= CHI_ARP_FULL)
        return 3u;
    return range + 1u;
}

/* MIDI ticks at 24 ppqn via clip CK_RATE_T (one rate table). */
static inline uint32_t chi_arp_ticks(uint32_t speed)
{
    if (speed >= CHI_ARP_NSPEED)
        speed = CHI_ARP_SPD_1_16;
    return CK_RATE_T[CHI_ARP_RATE_IX[speed]];
}

/* Map Chicago Down/Up/UpDown → clip CK_ORD_DOWN / CK_ORD_UP / CK_ORD_UPDN. */
static inline uint32_t chi_arp_order(uint32_t mode)
{
    if (mode == CHI_ARP_DOWN)
        return CK_ORD_DOWN;
    if (mode == CHI_ARP_UPDN)
        return CK_ORD_UPDN;
    return CK_ORD_UP;
}

/* Factory / AU 0..255 → discrete index 0..n-1 (round). */
static inline uint8_t chi_arp_from_byte(uint8_t v, uint32_t n)
{
    if (n <= 1u)
        return 0;
    return (uint8_t)(((uint32_t)v * (n - 1u) + 127u) / 255u);
}

static inline void chi_arp_decode_factory(const uint8_t pots[4], uint8_t out[4])
{
    out[0] = chi_arp_from_byte(pots[0], 2);
    out[1] = chi_arp_from_byte(pots[1], CHI_ARP_NMODE);
    out[2] = chi_arp_from_byte(pots[2], CHI_ARP_NRANGE);
    out[3] = chi_arp_from_byte(pots[3], CHI_ARP_NSPEED);
}

/* Apply factory pots[13..16] into a 303 voice sound cell (discrete indices). */
static inline void chi_arp_apply_factory_voice(uint8_t *voice_pots, const uint8_t factory_pots[4])
{
    uint8_t d[4];
    chi_arp_decode_factory(factory_pots, d);
    voice_pots[CHI_ARP_SWITCH] = d[0];
    voice_pots[CHI_ARP_MODE] = d[1];
    voice_pots[CHI_ARP_RANGE] = d[2];
    voice_pots[CHI_ARP_SPEED] = d[3];
}

/* Pick the MIDI note for arp step. s[0..n) sorted low→high.
 * Thin adapter onto clip ck_arp_pick (rnd unused — Chicago has no RANDOM). */
static inline int chi_arp_pick(const uint8_t *s, uint32_t n, uint32_t mode, uint32_t range,
                               uint32_t step)
{
    if (!s || n == 0)
        return -1;
    return ck_arp_pick(s, n, chi_arp_order(mode), chi_arp_octaves(range), step, 0u);
}

/* Samples per arp step at 24 ppqn and sample rate sr (Q0). */
static inline uint32_t chi_arp_samples_per_step(float bpm, uint32_t speed, float sr)
{
    float ticks = (float)chi_arp_ticks(speed);
    float sp = (sr * 60.0f * ticks) / (bpm * 24.0f);
    if (sp < 1.0f)
        sp = 1.0f;
    return (uint32_t)(sp + 0.5f);
}

/* Live held-note runtime. Engine and host tests share this. */
typedef struct {
    uint8_t notes[CHI_ARP_MAXN];
    uint8_t n, mode, range, speed, step;
    uint32_t to_next; /* samples until the next step; 0 = due now */
    int cur;          /* MIDI note sounding, or -1 */
} chi_arp_run_t;

static inline void chi_arp_run_clear(chi_arp_run_t *r)
{
    r->n = 0;
    r->step = 0;
    r->to_next = 0;
    r->cur = -1;
}

/* Replace the held pitch list (sorted low→high). Restarts the cycle.
 * Body lives in chicago_param_tables.c (one copy; was inlined twice in midi_in). */
void chi_arp_run_set(chi_arp_run_t *r, const uint8_t *notes, uint32_t n, uint8_t mode,
                     uint8_t range, uint8_t speed);

/* Single held pitch (common freeplay case). */
static inline void chi_arp_run_hold(chi_arp_run_t *r, uint8_t note, uint8_t mode, uint8_t range,
                                    uint8_t speed)
{
    chi_arp_run_set(r, &note, 1u, mode, range, speed);
}

/* Process samples at bpm/sr. Calls on_step for every due arp note in this block. */
static inline void chi_arp_run_process(chi_arp_run_t *r, uint32_t samples, float bpm, float sr,
                                       void (*on_step)(int note, void *ctx), void *ctx)
{
    uint32_t period;
    if (!r || !r->n || !on_step)
        return;
    if (bpm < 20.0f)
        bpm = 20.0f;
    if (bpm > 275.0f)
        bpm = 275.0f;
    period = chi_arp_samples_per_step(bpm, r->speed, sr);
    while (samples) {
        if (r->to_next == 0) {
            int n = chi_arp_pick(r->notes, r->n, r->mode, r->range, r->step++);
            r->to_next = period ? period : 1u;
            if (n >= 0) {
                r->cur = n;
                on_step(n, ctx);
            }
        }
        if (samples < r->to_next) {
            r->to_next -= samples;
            return;
        }
        samples -= r->to_next;
        r->to_next = 0;
    }
}
