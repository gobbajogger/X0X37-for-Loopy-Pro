#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Full audio CQ must retain drum hits (C_DRUM) under param flood.

Sibling of engine_cq_bon_overflow_test.py (#603). Extracts production cq_put
from engine.c. A full ring of params must still accept C_DRUM by displacing a
disposable param/drum entry. C_BOFF and C_BRK release remain protected.
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

static int expect_drum_kept(void)
{
    cq_w = cq_r = 0;
    for (unsigned i = 0; i < CQ; i++)
        cq_put(C_PARAM_T0, 0, 0, i);
    cq_put(C_DRUM, 0, 5, 1000);
    if (cq_w - cq_r != CQ)
        return 11;
    if (count_type(C_DRUM) != 1)
        return 12;
    if (count_type(C_PARAM_T0) != (int)CQ - 1)
        return 13;
    return 0;
}

static int expect_drum_does_not_displace_boff(void)
{
    cq_w = cq_r = 0;
    cq_put(C_BOFF, 0, 0, 0);
    for (unsigned i = 0; i < CQ - 1u; i++)
        cq_put(C_PARAM_T0, 0, 0, i);
    cq_put(C_DRUM, 1, 3, 2000);
    if (count_type(C_BOFF) != 1)
        return 21;
    if (count_type(C_DRUM) != 1)
        return 22;
    return 0;
}

static int expect_drum_does_not_displace_brk_release(void)
{
    cq_w = cq_r = 0;
    cq_put(C_BRK, 2, 0, 0);
    for (unsigned i = 0; i < CQ - 1u; i++)
        cq_put(C_PARAM_T0, 0, 0, i);
    cq_put(C_DRUM, 0, 7, 1500);
    if (count_brk_down(0) != 1)
        return 31;
    if (count_type(C_DRUM) != 1)
        return 32;
    return 0;
}

static int expect_param_still_drops(void)
{
    cq_w = cq_r = 0;
    for (unsigned i = 0; i < CQ; i++)
        cq_put(C_PARAM_T0, 0, 0, i);
    cq_put(C_PARAM_T0, 0, 0, 0xDEF);
    if (cq_w - cq_r != CQ)
        return 41;
    for (uint32_t i = cq_r; i != cq_w; i++) {
        if ((cq[i % CQ] & 0xFFFu) == 0xDEFu)
            return 42;
    }
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = expect_drum_kept()) != 0) {
        printf("FAIL drum-kept rc=%d depth=%u drum=%d param=%d\n",
               rc, (unsigned)(cq_w - cq_r), count_type(C_DRUM), count_type(C_PARAM_T0));
        return 1;
    }
    if ((rc = expect_drum_does_not_displace_boff()) != 0) {
        printf("FAIL drum-vs-boff rc=%d\n", rc);
        return 2;
    }
    if ((rc = expect_drum_does_not_displace_brk_release()) != 0) {
        printf("FAIL drum-vs-brk-rel rc=%d\n", rc);
        return 3;
    }
    if ((rc = expect_param_still_drops()) != 0) {
        printf("FAIL param-drop rc=%d\n", rc);
        return 4;
    }
    puts("ok");
    return 0;
}
"""
)
with tempfile.TemporaryDirectory(prefix="x0x-cq-drum-") as d:
    cpath = Path(d) / "t.c"
    cpath.write_text(src)
    exe = Path(d) / "t"
    subprocess.check_call(
        ["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", str(cpath), "-o", str(exe)]
    )
    raise SystemExit(subprocess.call([str(exe)]))
