/* SPDX-License-Identifier: GPL-3.0-only */
/* Reference: main_fm1.c at e9e6643e612b9fb4e968a77d016f3bb24213dc88.
 * Compare actual extracted candidate code, list bytes, MIDI event order and
 * arp state at identical logical clock/settings snapshots. No hardware timing. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define ck_arp_pick real_arp_pick
#include "app/smk_clip_arp_share.c"
#undef ck_arp_pick

typedef struct {
    uint8_t ck_scale, ck_root, ck_snap, ck_chord, ck_inv;
    uint8_t ck_ord, ck_oct, ck_gate;
} settings_t;
static struct { settings_t set; } proj;
typedef struct {
    uint8_t on, nk, korder[8], arp_note, gate_left;
} ck_t;
static ck_t ck;
static uint32_t fm1_ms;
#define CK_BASE ck_t *cp = &ck
#define CK_SET settings_t *sp = &proj.set

typedef struct {
    uint8_t list[CK_ARPN], n, events[4];
    unsigned nevents;
} trace_t;
static trace_t trace;
static unsigned calls, saved_max;
static uint32_t counted_key_notes(uint32_t k, uint8_t *out);
static void ck_arp_stop_note(void)
{
    if (ck.arp_note) {
        trace.events[trace.nevents++] = 0x80;
        trace.events[trace.nevents++] = ck.arp_note - 1u;
        ck.arp_note = 0;
    }
}
static void ck_on(uint32_t note)
{
    trace.events[trace.nevents++] = 0x90;
    trace.events[trace.nevents++] = (uint8_t)note;
}
static int ck_arp_pick(const uint8_t *s, uint32_t n, uint32_t order,
                       uint32_t oct, uint32_t step, uint32_t rnd)
{
    memcpy(trace.list, s, n);
    trace.n = (uint8_t)n;
    return real_arp_pick(s, n, order, oct, step, rnd);
}
#include "candidate.inc"
#undef ck_key_notes
static uint32_t counted_key_notes(uint32_t k, uint8_t *out)
{
    calls++;
    return ck_key_notes(k, out);
}

/* Frozen baseline. Keep the unbounded outer scan to detect any change to the
 * retained prefix, sort order, note-off/on sequence or gate state. */
static void reference_arp_step(uint32_t rate, uint32_t pos)
{
    CK_BASE;
    CK_SET;
    uint8_t list[CK_ARPN], out[CK_MAXN];
    uint32_t n = 0, i, j, c;
    int note;
    for (i = 0; i < cp->nk; i++)
        for (j = 0, c = counted_key_notes(cp->korder[i], out); j < c && n < CK_ARPN; j++) {
            uint32_t x = out[j], p = n++;
            if (sp->ck_ord != CK_ORD_PLAYED)
                for (; p && list[p - 1u] > x; p--)
                    list[p] = list[p - 1u];
            list[p] = (uint8_t)x;
        }
    ck_arp_stop_note();
    if (!n)
        return;
    note = ck_arp_pick(list, n, sp->ck_ord, sp->ck_oct + 1u, pos, fm1_ms * 2654435761u >> 7);
    if (note >= 0) {
        ck_on((uint32_t)note);
        cp->arp_note = (uint8_t)(note + 1);
        j = rate * (sp->ck_gate + 1u) / 10u;
        cp->gate_left = (uint8_t)(j ? j : 1u);
    }
}

static uint32_t rng = 1;
static uint32_t next(void) { rng = rng * 1664525u + 1013904223u; return rng; }
static void compare(uint32_t rate, uint32_t pos)
{
    ck_t input = ck, expected;
    trace_t expected_trace;
    unsigned old_calls;
    memset(&trace, 0, sizeof trace);
    calls = 0;
    reference_arp_step(rate, pos);
    expected = ck;
    expected_trace = trace;
    old_calls = calls;
    ck = input;
    memset(&trace, 0, sizeof trace);
    calls = 0;
    ck_arp_step(rate, pos);
    assert(memcmp(&ck, &expected, sizeof ck) == 0);
    assert(memcmp(&trace, &expected_trace, sizeof trace) == 0);
    assert(calls <= old_calls);
    if (old_calls - calls > saved_max) saved_max = old_calls - calls;
}
int main(void)
{
    unsigned cases = 0;
    for (unsigned type = 0; type < CK_NCHORD; type++)
        for (unsigned mode = 0; mode < 4; mode++)
            for (unsigned order = 0; order < CK_NORD; order++)
                for (unsigned nk = 0; nk <= 8; nk++)
                    for (unsigned sample = 0; sample < 64; sample++) {
                        proj.set = (settings_t){
                            .ck_scale = next() % CK_NSCALE, .ck_root = next() % 12,
                            .ck_snap = next() % 2, .ck_chord = type, .ck_inv = next() % 4,
                            .ck_ord = order, .ck_oct = next() % 4, .ck_gate = next() % 10
                        };
                        memset(&ck, 0, sizeof ck);
                        ck.on = mode;
                        ck.nk = nk;
                        ck.arp_note = next() % 129;
                        ck.gate_left = next() % 255;
                        for (unsigned k = 0; k < 8; k++) ck.korder[k] = next() % CK_NKEY;
                        fm1_ms = next();
                        compare(CK_RATE_T[next() % CK_NRATE], next());
                        cases++;
                    }
    assert(saved_max == 4); /* Eight held 7-note chords need only four expansions. */
    printf("PASS: %u arp cases; list/event/state bytes equal; max %u expansions saved\n",
           cases, saved_max);
    return 0;
}
