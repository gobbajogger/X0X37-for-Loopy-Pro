/* SPDX-License-Identifier: GPL-3.0-only */
/* Displacement appends protected packets after retained events, including wrap. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define RING_PUBLISH() ((void)0)
#include "usb_hygiene_production.h"

static uint32_t packet(unsigned cin, unsigned status, unsigned d1, unsigned d2)
{ return cin | status << 8 | d1 << 16 | d2 << 24; }
static uint32_t cc(unsigned i) { return packet(0xb, 0xb0, 1, i); }
static uint32_t note_off(void) { return packet(8, 0x80, 60, 0); }
static uint32_t stop(void) { return packet(0xf, 0xfc, 0, 0); }

static void ordered_two_displacements(uint32_t start, int retain_note_on)
{
    uint32_t q[MQ] = {0};
    volatile uint32_t r = start, w = start;
    uint32_t first = packet(9, 0x90, 60, 100);
    unsigned offset = retain_note_on ? 1u : 0u;
    if (retain_note_on) assert(midi_ring_put_note_off(q, &w, &r, first) == 1);
    for (unsigned i = 0; i < MQ - offset; i++)
        assert(midi_ring_put_note_off(q, &w, &r, cc(i)) == 1);
    uint32_t full_w = w;
    assert(midi_ring_put_note_off(q, &w, &r, note_off()) == 1);
    assert(midi_ring_put_note_off(q, &w, &r, stop()) == 1);
    assert(r == start && w == full_w && w - r == MQ);
    if (retain_note_on) assert(q[r % MQ] == first);
    for (unsigned i = offset; i < MQ - 2; i++)
        assert(q[(r + i) % MQ] == cc(i + 2 - offset));
    assert(q[(w - 2u) % MQ] == note_off());
    assert(q[(w - 1u) % MQ] == stop());
}
static void full_keep_ring_refuses_without_mutation(uint32_t start)
{
    uint32_t q[MQ], snapshot[MQ];
    volatile uint32_t r = start, w = start;
    for (unsigned i = 0; i < MQ; i++)
        assert(midi_ring_put_note_off(q, &w, &r, i & 1 ? stop() : note_off()) == 1);
    memcpy(snapshot, q, sizeof q);
    uint32_t full_w = w;
    assert(midi_ring_put_note_off(q, &w, &r, cc(99)) == 0);
    assert(r == start && w == full_w && memcmp(q, snapshot, sizeof q) == 0);
    assert(midi_ring_put_note_off(q, &w, &r, note_off()) == 0);
    assert(r == start && w == full_w && memcmp(q, snapshot, sizeof q) == 0);
}
int main(void)
{
    const uint32_t starts[] = {0, MQ - 3u, UINT32_MAX - 31u};
    for (unsigned i = 0; i < sizeof starts / sizeof starts[0]; i++) {
        ordered_two_displacements(starts[i], 0);
        ordered_two_displacements(starts[i], 1);
        full_keep_ring_refuses_without_mutation(starts[i]);
    }
    puts("PASS MIDI ring ordered Note Off/Stop displacement and explicit full-keep refusal (loader)");
    return 0;
}
