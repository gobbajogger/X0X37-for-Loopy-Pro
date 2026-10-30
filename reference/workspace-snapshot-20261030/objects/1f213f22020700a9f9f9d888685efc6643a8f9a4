#!/usr/bin/env python3
"""Full audio CQ must retain bass Note Off (C_BOFF) under param flood.

Extracts production cq_put from engine.c. A queued C_BON plus a full ring of
params must still accept C_BOFF by displacing a disposable param/drum entry.
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

static int expect_boff_kept(void)
{
    cq_w = cq_r = 0;
    cq_put(C_BON, 0, 60, 400);
    for (unsigned i = 0; i < CQ - 1u; i++)
        cq_put(C_PARAM_T0, 0, 0, i);
    cq_put(C_BOFF, 0, 0, 0);
    if (cq_w - cq_r != CQ)
        return 11;
    if (count_type(C_BON) != 1)
        return 12;
    if (count_type(C_BOFF) != 1)
        return 13;
    return 0;
}

static int expect_param_still_drops(void)
{
    cq_w = cq_r = 0;
    for (unsigned i = 0; i < CQ; i++)
        cq_put(C_PARAM_T0, 0, 0, i);
    cq_put(C_PARAM_T0, 0, 0, 0xABC);
    if (cq_w - cq_r != CQ)
        return 21;
    if (count_type(C_PARAM_T0) != (int)CQ)
        return 22;
    /* Last write must have been dropped — no 0xABC in low 12 bits of any slot. */
    for (uint32_t i = cq_r; i != cq_w; i++) {
        if ((cq[i % CQ] & 0xFFFu) == 0xABCu)
            return 23;
    }
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = expect_boff_kept()) != 0) {
        printf("FAIL boff-kept rc=%d depth=%u bon=%d boff=%d\n",
               rc, (unsigned)(cq_w - cq_r), count_type(C_BON), count_type(C_BOFF));
        return 1;
    }
    if ((rc = expect_param_still_drops()) != 0) {
        printf("FAIL param-drop rc=%d\n", rc);
        return 2;
    }
    puts("ok");
    return 0;
}
"""
)
with tempfile.TemporaryDirectory(prefix="x0x-cq-boff-") as d:
    cpath = Path(d) / "t.c"
    cpath.write_text(src)
    exe = Path(d) / "t"
    subprocess.check_call(
        ["cc", "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", str(cpath), "-o", str(exe)]
    )
    raise SystemExit(subprocess.call([str(exe)]))
