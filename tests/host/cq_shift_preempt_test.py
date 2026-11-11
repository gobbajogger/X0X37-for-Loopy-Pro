#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""A command-queue replace copies one slot per masked step.

An audio-side walk runs at every unlock and between the copy and the hole
publish. It must never see a duplicated or missing command. Device and native
builds are not this test.
"""
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

PRE = r"""
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static int masked, torn, probes;
static uint8_t seen[16u * 4096u];
static int live_dups(void);
static void shift_probe(void);
static uint32_t plat_cq_lock(void) { masked = 1; return 0; }
static void plat_cq_unlock(uint32_t s);
"""

HARNESS = r"""
static int live_dups(void)
{
    uint32_t k, n = 0;
    memset(seen, 0, sizeof seen);
    for (k = cq_r; k != cq_w; k++) {
        uint32_t word, slot;
        if (cq_hole_on && k == cq_hole_at)
            continue;
        word = cq[k % CQ];
        slot = ((word >> 28) << 12) | (word & 0xFFFu);
        n += seen[slot];
        seen[slot] = 1;
    }
    return (int)n;
}
static void shift_probe(void)
{
    probes++;
    /* Audio is masked here. A probe that runs with the source enabled can
     * observe the copy before the hole is published. */
    if (!masked)
        torn += live_dups();
}
static void plat_cq_unlock(uint32_t s)
{
    (void)s;
    masked = 0;
    torn += live_dups();
}
static int expect_shift(void)
{
    uint32_t k, bon = 0, boff = 0;
    cq_w = cq_r = 0;
    cq_hole_on = 0;
    cq_put(C_BOFF, 0, 0, 0);
    for (k = 0; k < CQ - 1u; k++)
        cq_put(C_PARAM_T0, 0, 0, k);
    cq_put(C_BON, 1, 60, 0x345);
    if (probes == 0 || torn != 0 || cq_w - cq_r != CQ)
        return 1;
    for (k = cq_r; k != cq_w; k++) {
        uint32_t t = cq[k % CQ] >> 28;
        bon += t == C_BON;
        boff += t == C_BOFF;
    }
    if (bon != 1 || boff != 1)
        return 2;
    if ((cq[cq_r % CQ] >> 28) != C_BOFF)
        return 3;
    if ((cq[(cq_w - 1u) % CQ] >> 28) != C_BON)
        return 4;
    return 0;
}
int main(void)
{
    int rc = expect_shift();
    if (rc) {
        printf("FAIL cq shift preempt rc=%d probes=%d torn=%d\n", rc, probes, torn);
        return 1;
    }
    printf("cq shift preempt: 0 failures\n");
    return 0;
}
"""


def main():
    engine = (ROOT / "firmware/src/app/engine.c").read_text()
    start = engine.index("enum { C_PARAM_T0 = 0, C_DRUM")
    end = engine.index("/* ------------------------------------------------------------ params --- */", start)
    ring = engine[start:end]
    copy = "                cq[i % CQ] = cq[src % CQ];\n"
    hole = "                cq_hole_at = src;\n"
    pair = copy + hole
    if ring.count(pair) != 1:
        raise SystemExit("shift anchor missing")
    ring = ring.replace(pair, copy + "                shift_probe();\n" + hole, 1)
    source = PRE + ring + HARNESS
    with tempfile.TemporaryDirectory(prefix="x0x-cq-shift-") as tmp:
        c = Path(tmp) / "test.c"
        exe = Path(tmp) / "test"
        c.write_text(source)
        subprocess.run(
            ["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", str(c), "-o", str(exe)],
            check=True,
        )
        return subprocess.run([str(exe)]).returncode


if __name__ == "__main__":
    raise SystemExit(main())
