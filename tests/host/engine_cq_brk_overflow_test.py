#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Full audio CQ must retain BREAK key press and release (C_BRK) under param flood.

Sibling of engine_cq_boff_overflow_test.py (#588) / bon (#603) / brk-press (#605).
Extracts production cq_put from engine.c. A full ring of params must still
accept a C_BRK press by displacing a disposable param/drum entry; a queued
press plus a full ring must still accept a C_BRK release. C_BOFF and an
already-queued C_BRK release must not be displaced by a later press either.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
e = (ROOT / "firmware/src/app/engine.c").read_text()
ring = e[e.index("enum { C_PARAM_T0 = 0, C_DRUM") : e.index("/* ------------------------------------------------------------ params --- */")]
src = (
    "#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n"
    + "static uint32_t plat_cq_lock(void) { return 0; }\nstatic void plat_cq_unlock(uint32_t s) { (void)s; }\n"
    + ring
    + r"""
static int count_type(uint32_t type)
{
    unsigned n = 0;
    for (uint32_t i = cq_r; i != cq_w; i++)
        n += (cq[i % CQ] >> 28) == type;
    return (int)n;
}

static int count_brk_down(uint32_t down)
{
    unsigned n = 0;
    for (uint32_t i = cq_r; i != cq_w; i++) {
        uint32_t w = cq[i % CQ];
        if ((w >> 28) == C_BRK && ((w >> 12) & 0xFFu) == down)
            n++;
    }
    return (int)n;
}

static int expect_brk_press_kept(void)
{
    cq_w = cq_r = 0;
    for (unsigned i = 0; i < CQ; i++)
        cq_put(C_PARAM_T0, 0, 0, i);
    cq_put(C_BRK, 3, 1, 0);                 /* press must displace a param */
    if (cq_w - cq_r != CQ)
        return 11;
    if (count_brk_down(1) != 1)
        return 12;
    if (count_type(C_PARAM_T0) != (int)CQ - 1)
        return 13;
    return 0;
}

static int expect_brk_release_kept(void)
{
    cq_w = cq_r = 0;
    cq_put(C_BRK, 3, 1, 0);                 /* press slice 3 */
    for (unsigned i = 0; i < CQ - 1u; i++)
        cq_put(C_PARAM_T0, 0, 0, i);
    cq_put(C_BRK, 3, 0, 0);                 /* release must displace a param */
    if (cq_w - cq_r != CQ)
        return 21;
    if (count_brk_down(1) != 1)
        return 22;
    if (count_brk_down(0) != 1)
        return 23;
    if (count_type(C_PARAM_T0) != (int)CQ - 2)
        return 24;
    return 0;
}

static int expect_boff_still_kept(void)
{
    cq_w = cq_r = 0;
    cq_put(C_BON, 0, 60, 400);
    for (unsigned i = 0; i < CQ - 1u; i++)
        cq_put(C_PARAM_T0, 0, 0, i);
    cq_put(C_BOFF, 0, 0, 0);
    if (count_type(C_BON) != 1)
        return 31;
    if (count_type(C_BOFF) != 1)
        return 32;
    return 0;
}

static int expect_release_does_not_displace_boff(void)
{
    cq_w = cq_r = 0;
    cq_put(C_BOFF, 0, 0, 0);
    cq_put(C_BRK, 1, 1, 0);
    for (unsigned i = 0; i < CQ - 2u; i++)
        cq_put(C_PARAM_T0, 0, 0, i);
    /* Ring full of: 1×C_BOFF, 1×C_BRK press, rest params. Release must displace a param. */
    cq_put(C_BRK, 1, 0, 0);
    if (count_type(C_BOFF) != 1)
        return 41;
    if (count_brk_down(1) != 1)
        return 42;
    if (count_brk_down(0) != 1)
        return 43;
    return 0;
}

static int expect_press_does_not_displace_boff(void)
{
    cq_w = cq_r = 0;
    cq_put(C_BOFF, 0, 0, 0);
    for (unsigned i = 0; i < CQ - 1u; i++)
        cq_put(C_PARAM_T0, 0, 0, i);
    cq_put(C_BRK, 2, 1, 0);
    if (count_type(C_BOFF) != 1)
        return 51;
    if (count_brk_down(1) != 1)
        return 52;
    return 0;
}

static int expect_press_does_not_displace_brk_release(void)
{
    cq_w = cq_r = 0;
    cq_put(C_BRK, 4, 0, 0);                 /* release already queued */
    for (unsigned i = 0; i < CQ - 1u; i++)
        cq_put(C_PARAM_T0, 0, 0, i);
    cq_put(C_BRK, 5, 1, 0);
    if (count_brk_down(0) != 1)
        return 61;
    if (count_brk_down(1) != 1)
        return 62;
    return 0;
}

static int expect_param_still_drops(void)
{
    cq_w = cq_r = 0;
    for (unsigned i = 0; i < CQ; i++)
        cq_put(C_PARAM_T0, 0, 0, i);
    cq_put(C_PARAM_T0, 0, 0, 0xABC);
    if (cq_w - cq_r != CQ)
        return 71;
    for (uint32_t i = cq_r; i != cq_w; i++) {
        if ((cq[i % CQ] & 0xFFFu) == 0xABCu)
            return 72;
    }
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = expect_brk_press_kept()) != 0) {
        printf("FAIL brk-press-kept rc=%d depth=%u press=%d param=%d\n",
               rc, (unsigned)(cq_w - cq_r), count_brk_down(1), count_type(C_PARAM_T0));
        return 1;
    }
    if ((rc = expect_brk_release_kept()) != 0) {
        printf("FAIL brk-release-kept rc=%d depth=%u press=%d rel=%d\n",
               rc, (unsigned)(cq_w - cq_r), count_brk_down(1), count_brk_down(0));
        return 2;
    }
    if ((rc = expect_boff_still_kept()) != 0) {
        printf("FAIL boff-still-kept rc=%d\n", rc);
        return 3;
    }
    if ((rc = expect_release_does_not_displace_boff()) != 0) {
        printf("FAIL brk-vs-boff rc=%d\n", rc);
        return 4;
    }
    if ((rc = expect_press_does_not_displace_boff()) != 0) {
        printf("FAIL press-vs-boff rc=%d\n", rc);
        return 5;
    }
    if ((rc = expect_press_does_not_displace_brk_release()) != 0) {
        printf("FAIL press-vs-brk-rel rc=%d\n", rc);
        return 6;
    }
    if ((rc = expect_param_still_drops()) != 0) {
        printf("FAIL param-drop rc=%d\n", rc);
        return 7;
    }
    puts("ok");
    return 0;
}
"""
)
with tempfile.TemporaryDirectory(prefix="x0x-cq-brk-") as d:
    cpath = Path(d) / "t.c"
    cpath.write_text(src)
    exe = Path(d) / "t"
    subprocess.check_call(
        ["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", str(cpath), "-o", str(exe)]
    )
    raise SystemExit(subprocess.call([str(exe)]))
