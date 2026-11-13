/* SPDX-License-Identifier: GPL-3.0-only */
/* Reference vs compact outer-key state machine. Not a whole-firmware test. */
#include <stdint.h>
#include <stdio.h>
#include "rec_outer_keymap.h"

static const unsigned physical_key[10] = {0,1,2,3,4,32,33,34,35,36};
static uint32_t rng = UINT32_C(0x37a0c13);
static uint32_t next_random(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

static uint64_t expand(uint16_t packed)
{
    uint64_t physical = 0;
    for (unsigned i = 0; i < 10; ++i)
        if (packed & (uint16_t)(1u << i))
            physical |= UINT64_C(1) << physical_key[i];
    return physical;
}

/* Models intentionally have separate field widths; only event masks are compared. */
typedef struct { uint64_t prev, suppress, admitted; unsigned armed; } wide_t;
typedef struct { uint16_t prev, suppress, admitted; unsigned armed; } compact_t;

static void wide_step(wide_t *st, uint64_t phys, int enabled,
                      uint16_t *rise, uint16_t *fall)
{
    *rise = *fall = 0;
    if (!enabled) {
        for (unsigned i = 0; i < 10; ++i)
            if (st->admitted & (UINT64_C(1) << physical_key[i]))
                *fall |= (uint16_t)(1u << i);
        st->admitted = 0;
        st->prev = st->suppress = phys;
        st->armed = 0;
        return;
    }
    if (!st->armed) {
        st->suppress = st->prev = phys;
        st->armed = 1;
        return;
    }
    for (unsigned i = 0; i < 10; ++i) {
        uint64_t bit = UINT64_C(1) << physical_key[i];
        int on = !!(phys & bit), was = !!(st->prev & bit);
        if (on && (st->suppress & bit)) continue;
        if (!on) st->suppress &= ~bit;
        if (on && !was) {
            *rise |= (uint16_t)(1u << i);
            st->admitted |= bit;
        } else if (!on && was && (st->admitted & bit)) {
            *fall |= (uint16_t)(1u << i);
            st->admitted &= ~bit;
        }
    }
    st->prev = phys;
}

static void compact_step(compact_t *st, uint64_t raw, int enabled,
                         uint16_t *rise, uint16_t *fall)
{
    uint16_t phys = rec_outer_bits(raw);
    *rise = *fall = 0;
    if (!enabled) {
        *fall = st->admitted;
        st->admitted = 0;
        st->prev = st->suppress = phys;
        st->armed = 0;
        return;
    }
    if (!st->armed) {
        st->suppress = st->prev = phys;
        st->armed = 1;
        return;
    }
    for (unsigned i = 0; i < 10; ++i) {
        uint16_t bit = (uint16_t)(1u << i);
        int on = !!(phys & bit), was = !!(st->prev & bit);
        if (on && (st->suppress & bit)) continue;
        if (!on) st->suppress &= (uint16_t)~bit;
        if (on && !was) {
            *rise |= bit;
            st->admitted |= bit;
        } else if (!on && was && (st->admitted & bit)) {
            *fall |= bit;
            st->admitted &= (uint16_t)~bit;
        }
    }
    st->prev = phys;
}

int main(void)
{
    unsigned long checks = 0, rises = 0, falls = 0;
    for (unsigned i = 0; i < 10; ++i) {
        if (rec_outer_physical(i) != physical_key[i]) return 1;
        ++checks;
    }
    for (unsigned bits = 0; bits < 1024; ++bits) {
        uint64_t base = expand((uint16_t)bits);
        for (unsigned j = 0; j < 8; ++j) {
            /* The entire FM-1 window may vary without affecting outer REC. */
            uint64_t central = ((uint64_t)next_random() << 5) &
                               ((UINT64_C(1) << 32) - (UINT64_C(1) << 5));
            if (rec_outer_bits(base | central) != (uint16_t)bits) return 2;
            ++checks;
        }
    }
    wide_t wide = {0}; compact_t compact = {0};
    uint16_t held = 0;
    for (unsigned step = 0; step < 250000; ++step) {
        /* Include rec entry/exit, long holds, simultaneous keys, central noise. */
        uint32_t rnd = next_random();
        if (!(rnd & 15u)) held = (uint16_t)(next_random() & 1023u);
        else if (rnd & 1u) held ^= (uint16_t)(1u << ((rnd >> 5) % 10u));
        uint64_t raw = expand(held) |
                       ((((uint64_t)next_random() << 5)) &
                        ((UINT64_C(1) << 32) - (UINT64_C(1) << 5)));
        int enabled = ((step / 37u) % 5u) != 0;
        uint16_t wr, wf, cr, cf;
        wide_step(&wide, raw, enabled, &wr, &wf);
        compact_step(&compact, raw, enabled, &cr, &cf);
        if (wr != cr || wf != cf || wide.armed != compact.armed ||
            rec_outer_bits(wide.admitted) != compact.admitted ||
            rec_outer_bits(wide.suppress) != compact.suppress ||
            rec_outer_bits(wide.prev) != compact.prev) {
            fprintf(stderr,"mismatch at step %u: rise %03x/%03x fall %03x/%03x\n",
                    step, wr, cr, wf, cf);
            return 3;
        }
        rises += __builtin_popcount((unsigned)cr);
        falls += __builtin_popcount((unsigned)cf);
        ++checks;
    }
    if (!rises || !falls) return 4;
    printf("rec_outer_compact PASS checks=%lu rises=%lu falls=%lu\n", checks, rises, falls);
    return 0;
}
