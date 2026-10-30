/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>

/* Mutation exclusion bookkeeping only. No transport, raw reader or boot writer.
 * Core-0 main-loop ownership, no concurrent/ISR acquisition. Caller must prove
 * global quiescence before acquisition; this is not that proof or a spinlock.
 * Owner is a local opaque lease handle, NEVER physical-unit identity. */
struct fm1_capture_gate {
    uint64_t owner, epoch, captured_epoch;
    uint8_t held, revoked, exhausted;
};

enum { FM1_CAPTURE_UNSUPPORTED = -20, FM1_CAPTURE_BUSY = -21 };

static inline int fm1_capture_gate_acquire(struct fm1_capture_gate *g,
                                           uint64_t owner, int qualified, int quiescent)
{
    if (!g || !owner || qualified != 1 || quiescent != 1 || g->held || g->exhausted)
        return FM1_CAPTURE_BUSY;
    g->owner = owner;
    g->captured_epoch = g->epoch;
    g->revoked = 0;
    g->held = 1;
    return 0;
}

static inline int fm1_capture_gate_current(const struct fm1_capture_gate *g, uint64_t owner)
{
    return g && owner && g->held && !g->revoked && !g->exhausted &&
           g->owner == owner && g->epoch == g->captured_epoch;
}

static inline void fm1_capture_gate_revoke(struct fm1_capture_gate *g)
{
    if (g && g->held)
        g->revoked = 1;
}

static inline int fm1_capture_gate_release(struct fm1_capture_gate *g, uint64_t owner)
{
    if (!g || !owner || !g->held || g->owner != owner)
        return FM1_CAPTURE_BUSY;
    /* Release only after the caller's in-flight synchronous read has returned.
     * A revoked lease stays locked until this explicit owner release. */
    g->held = 0;
    g->revoked = 0;
    g->owner = 0;
    g->captured_epoch = 0;
    return 0;
}

static inline int fm1_capture_gate_mutation(struct fm1_capture_gate *g)
{
    if (!g)
        return 0;
    if (g->held) {
        g->revoked = 1;
        return 0; /* refuse this attempt; no erase/program or blind replay */
    }
    /* Saturate, never wrap a stale epoch into validity. Ordinary updates remain
     * available; only future capture admission is disabled at exhaustion. */
    if (g->epoch == UINT64_MAX)
        g->exhausted = 1;
    else
        ++g->epoch;
    return 1;
}

/* One state per single-unit firmware image (x0x.c or the separate app loader).
 * The application has no supported acquisition path: do not advertise capture.
 * These hooks nevertheless guard the actual common mutators/reset boundary.
 * A future adapter must qualify all bypasses/cores and pair lease lifecycle
 * with capture_core identity/deadline checks before adding any acquisition call. */
#if defined(BOARD_SMK37) && BOARD_SMK37 && !defined(FELUCCA_LOADER)
/* App .data/.bss is nearly full. Existing startup clears .pool; the separate
 * RAM-only loader has no pool and retains ordinary local zeroed storage. */
static struct fm1_capture_gate fm1_capture_barrier __attribute__((section(".pool")));
#else
static struct fm1_capture_gate fm1_capture_barrier;
#endif

static inline int fm1_capture_supported(void) { return 0; }
static inline int fm1_capture_begin(uint64_t owner)
{
    (void)owner;
    return FM1_CAPTURE_UNSUPPORTED;
}
static inline int fm1_capture_maintenance_held(void)
{
    return fm1_capture_barrier.held != 0;
}
static inline int fm1_capture_pre_admit(void)
{
    if (!fm1_capture_maintenance_held())
        return 1;
    fm1_capture_gate_revoke(&fm1_capture_barrier);
    return 0;
}
static inline int fm1_capture_mutation_allowed(void)
{
    return fm1_capture_gate_mutation(&fm1_capture_barrier);
}
static inline void fm1_capture_handoff(void)
{
    /* Reset is not blocked or turned into a falsely successful return. It
     * terminates this session; revoke before mailbox/reset side effects. A fresh
     * boot must establish a new independently qualified session before reuse. */
    fm1_capture_gate_revoke(&fm1_capture_barrier);
}
