/* SPDX-License-Identifier: GPL-3.0-only */
#include "fm1_capture_gate.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    struct fm1_capture_gate g = {0};
    unsigned i, groups = 0;
    assert(!fm1_capture_supported());
    assert(fm1_capture_begin(1) == FM1_CAPTURE_UNSUPPORTED);
    assert(!fm1_capture_barrier.held);
    assert(fm1_capture_gate_acquire(&g, 1, 2, 1) == FM1_CAPTURE_BUSY);
    assert(fm1_capture_gate_acquire(&g, 1, 1, -1) == FM1_CAPTURE_BUSY); groups++;
    for (i = 0; i < 4; ++i)
        assert(fm1_capture_gate_acquire(&g, i == 0 ? 0 : 1, i != 1, i != 2) ==
               (i == 3 ? 0 : FM1_CAPTURE_BUSY));
    assert(fm1_capture_gate_current(&g, 1)); groups++;
    assert(fm1_capture_gate_acquire(&g, 2, 1, 1) == FM1_CAPTURE_BUSY);
    assert(g.owner == 1 && fm1_capture_gate_current(&g, 1));
    assert(fm1_capture_gate_release(&g, 2) == FM1_CAPTURE_BUSY);
    assert(fm1_capture_gate_current(&g, 1)); groups++;
    assert(!fm1_capture_gate_mutation(&g));
    assert(g.held && g.revoked && !fm1_capture_gate_current(&g, 1));
    assert(!fm1_capture_gate_mutation(&g));
    assert(!fm1_capture_gate_release(&g, 1));
    assert(fm1_capture_gate_mutation(&g));
    assert(g.epoch == 1); groups++;
    assert(!fm1_capture_gate_acquire(&g, 3, 1, 1));
    assert(g.captured_epoch == 1);
    fm1_capture_gate_revoke(&g);
    assert(!fm1_capture_gate_current(&g, 3));
    assert(g.held);
    assert(!fm1_capture_gate_release(&g, 3)); groups++;
    g.epoch = UINT64_MAX;
    assert(fm1_capture_gate_mutation(&g));
    assert(g.exhausted && g.epoch == UINT64_MAX);
    assert(fm1_capture_gate_mutation(&g));
    assert(fm1_capture_gate_acquire(&g, 4, 1, 1) == FM1_CAPTURE_BUSY); groups++;
    /* White-box injection only, not a production acquisition entry point. */
    assert(!fm1_capture_gate_acquire(&fm1_capture_barrier, 5, 1, 1));
    assert(!fm1_capture_mutation_allowed());
    assert(!fm1_capture_gate_current(&fm1_capture_barrier, 5));
    assert(!fm1_capture_gate_release(&fm1_capture_barrier, 5));
    assert(fm1_capture_mutation_allowed()); groups++;
    assert(!fm1_capture_gate_acquire(&fm1_capture_barrier, 6, 1, 1));
    fm1_capture_handoff();
    assert(!fm1_capture_gate_current(&fm1_capture_barrier, 6));
    assert(fm1_capture_barrier.held); groups++;
    assert(!fm1_capture_gate_mutation(NULL));
    assert(!fm1_capture_gate_current(NULL, 1));
    assert(fm1_capture_gate_release(NULL, 1) == FM1_CAPTURE_BUSY);
    fm1_capture_gate_revoke(NULL); groups++;
    printf("PASS: %u mutation gate groups; platform OFF\n", groups);
    return 0;
}
