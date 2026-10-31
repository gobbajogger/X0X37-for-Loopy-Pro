/* SPDX-License-Identifier: GPL-3.0-only */
#include "production.h"

static uint32_t cc(unsigned value) { return 0xbu | 0xb0u << 8 | 7u << 16 | value << 24; }
static uint32_t bend(unsigned cable, unsigned channel, unsigned value)
{ return 0xeu | cable << 4 | (0xe0u + channel) << 8 | value << 24; }
static uint32_t off(void) { return 8u | 0x80u << 8 | 60u << 16; }
static uint32_t stop(void) { return 0xfu | 0xfcu << 8; }

static int enqueue(uint32_t p)
{
    uint32_t before = enabled;
    unsigned n = saves;
    int accepted = send(p);
    assert(enabled == before && !held && saves == n + 1 && saves == restores);
    return accepted;
}
static unsigned count(uint32_t packet)
{
    unsigned n = 0;
    for (uint32_t i = R; i != W; i++) n += Q[i % CAP] == packet;
    return n;
}
static void reset(uint32_t start)
{ memset(Q, 0, sizeof Q); R = W = start; assert(!held); saves = restores = 0; }
static void fill_cc(uint32_t start)
{
    reset(start);
    for (unsigned i = 0; i < CAP; i++) assert(enqueue(cc(i)));
    assert(W - R == CAP);
}
static void flood_retains_latest(uint32_t start)
{
    fill_cc(start);
    assert(enqueue(bend(0, 1, 17)));
    assert(count(bend(0, 1, 17)) == 1); /* RED: CC flood discarded PB. */
    assert(enqueue(bend(0, 1, 99)));
    assert(!count(bend(0, 1, 17)) && count(bend(0, 1, 99)) == 1);
    assert(enqueue(bend(0, 2, 55)));
    assert(enqueue(bend(1, 1, 77)));
    assert(count(bend(0, 1, 99)) == 1 && count(bend(0, 2, 55)) == 1 && count(bend(1, 1, 77)) == 1);
    assert(W - R == CAP);
    /* Only the first three CC slots were replaced/coalesced. */
    for (unsigned i = 3; i < CAP; i++) assert(Q[(R + i) % CAP] == cc(i));
    uint32_t snapshot[CAP]; memcpy(snapshot, Q, sizeof Q);
    int accepted = enqueue(cc(111));
#ifdef HAS_RESULT
    assert(!accepted);
#else
    (void)accepted;
#endif
    assert(!memcmp(snapshot, Q, sizeof Q));
}
static void release_and_transport_take_priority(uint32_t start)
{
    reset(start);
    for (unsigned i = 0; i < CAP; i++) assert(enqueue(bend(0, i % 16u, i)));
    assert(enqueue(off()) && enqueue(stop()));
    assert(W - R == CAP && count(off()) == 1 && count(stop()) == 1);
    for (unsigned i = 0; i < CAP - 2; i++)
        assert(Q[(R + i) % CAP] == bend(0, (i + 2) % 16u, i + 2));
    assert(Q[(W - 2u) % CAP] == off() && Q[(W - 1u) % CAP] == stop());
}
static void newest_queued_bend_is_updated(uint32_t start)
{
    reset(start);
    for (unsigned i = 0; i < CAP; i++) assert(enqueue(bend(0, 3, i)));
    assert(enqueue(bend(0, 3, 100)));
    assert(Q[(W - 1u) % CAP] == bend(0, 3, 100));
    for (unsigned i = 0; i < CAP - 1u; i++)
        assert(Q[(R + i) % CAP] == bend(0, 3, i));
}
static void bend_cannot_replace_keep_or_other_source(uint32_t start)
{
    reset(start);
    for (unsigned i = 0; i < CAP; i++) assert(enqueue(i & 1u ? off() : stop()));
    uint32_t snapshot[CAP]; memcpy(snapshot, Q, sizeof Q);
    int accepted = enqueue(bend(0, 0, 88));
#ifdef HAS_RESULT
    assert(!accepted);
#else
    (void)accepted;
#endif
    assert(!memcmp(snapshot, Q, sizeof Q) && W - R == CAP);
    reset(start);
    assert(enqueue(bend(2, 3, 42)));
    for (unsigned i = 1; i < CAP; i++) assert(enqueue(off()));
    memcpy(snapshot, Q, sizeof Q);
    accepted = enqueue(bend(1, 3, 84));
#ifdef HAS_RESULT
    assert(!accepted);
#endif
    assert(!memcmp(snapshot, Q, sizeof Q));
    assert(enqueue(bend(2, 3, 85)));
    assert(count(bend(2, 3, 85)) == 1 && count(off()) == CAP - 1);
}
#ifdef SUPPORT_LINK_END
static void link_end_survives_busy_input(uint32_t start)
{
    reset(start);
    assert(enqueue(0x20u)); /* MID-15 source 2 ended; no MIDI status payload. */
    for (unsigned i = 1; i < CAP; i++) assert(enqueue(i & 1u ? off() : stop()));
    uint32_t snapshot[CAP]; memcpy(snapshot, Q, sizeof Q);
    int accepted = enqueue(bend(0, 0, 88));
#ifdef HAS_RESULT
    assert(!accepted);
#else
    (void)accepted;
#endif
    assert(!memcmp(snapshot, Q, sizeof Q));
    accepted = enqueue(off());
#ifdef HAS_RESULT
    assert(!accepted); /* RED: the link-end marker was considered disposable. */
#endif
    assert(!memcmp(snapshot, Q, sizeof Q) && W - R == CAP);
    reset(start);
    assert(enqueue(0x20u));
    for (unsigned i = 1; i < CAP; i++) assert(enqueue(bend(0, 0, i)));
    assert(enqueue(off()));
    assert(count(0x20u) == 1 && count(off()) == 1);
}
#endif
int main(void)
{
    uint32_t starts[] = {0, CAP - 3u, UINT32_MAX - 31u};
    for (unsigned irq = 0; irq < 2; irq++) {
        enabled = irq;
        for (unsigned i = 0; i < sizeof starts / sizeof starts[0]; i++) {
            flood_retains_latest(starts[i]);
            release_and_transport_take_priority(starts[i]);
            newest_queued_bend_is_updated(starts[i]);
            bend_cannot_replace_keep_or_other_source(starts[i]);
#ifdef SUPPORT_LINK_END
            link_end_survives_busy_input(starts[i]);
#endif
        }
    }
    puts("PASS production Pitch Bend CC flood/coalescing/source isolation/keep priority/wrap/IRQ guards");
}
