#!/usr/bin/env python3
"""Compile/run exact bus-FX output/state comparison against the pre-patch Git blob.

Run in an authorized host-test environment: python3 tests/busfx_equivalence_test.py
Requires the pinned baseline object, Python 3, and a C99 compiler ($CC or cc).
No firmware binaries or generated reference files are retained in the repository.
"""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASE = "b2b929f46f1b7868dfa569d0d0989f5ab91aa16e"
SOURCE = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "reference.h"
#include "busfx.h"
static uint32_t rng = 0x913abc21u;
static float noise(void) {
    rng = rng * 1664525u + 1013904223u;
    return ((float)(rng >> 8) / 8388608.0f - 1.0f) * 1.5f;
}
int main(void) {
    const float cuts[] = {-1.0f, 0.0f, 40.0f, 100.0f, 126.49f, 126.5f, 127.0f};
    const float bpms[] = {0.0f, 20.0f, 97.3f, 120.0f, 299.9f, 300.0f, 400.0f};
    const int sizes[] = {-1, 0, 1, 2, 31, 64, 255, 256};
    unsigned cases = 0;
    for (int effect = 0; effect < BFX_NFX; ++effect)
    for (int nch = 1; nch <= 4; ++nch)
    for (unsigned ci = 0; ci < sizeof cuts / sizeof *cuts; ++ci)
    for (unsigned bi = 0; bi < sizeof bpms / sizeof *bpms; ++bi)
    for (int variant = 0; variant < 4; ++variant) {
        busfx_t s; ref_busfx_t r;
        busfx_stut_t st; ref_busfx_stut_t rt;
        busfx_init(&s); ref_busfx_init(&r);
        busfx_stut_init(&st); ref_busfx_stut_init(&rt);
        /* Carry state across calls, including release tails and sample wrap. */
        for (unsigned block = 0; block < 16; ++block) {
            float a[4][256], b[4][256];
            for (int k = 0; k < 4; ++k)
                for (int i = 0; i < 256; ++i)
                    a[k][i] = block > 11 ? 0.0f : noise();
            memcpy(b, a, sizeof a);
            int n = sizes[block % 8];
            int press = variant == 0 ? -1 : variant == 1 ? 0 : n / 2;
            int release = variant == 3 ? press : variant == 2 ? n - 1 : -1;
            uint32_t sample = block < 8 ? block * 33075u : UINT32_MAX - 127u;
            busfx_apply(&s, variant == 0 ? NULL : &st, variant * 4,
                        a[0], a[1], a[2], a[3], nch, n, cuts[ci], block < 10,
                        effect, press, release, bpms[bi], sample);
            ref_busfx_apply(&r, variant == 0 ? NULL : &rt, variant * 4,
                            b[0], b[1], b[2], b[3], nch, n, cuts[ci], block < 10,
                            effect, press, release, bpms[bi], sample);
            if (sizeof s != sizeof r || sizeof st != sizeof rt ||
                memcmp(a, b, sizeof a) || memcmp(&s, &r, sizeof s) ||
                memcmp(&st, &rt, sizeof st)) {
                fprintf(stderr, "FAIL effect=%d nch=%d cut=%g bpm=%g variant=%d block=%u\
",
                        effect, nch, cuts[ci], bpms[bi], variant, block);
                return 1;
            }
            ++cases;
        }
    }
    printf("PASS: %u bus-FX calls; audio, filter/gate state and ring bytes identical\
", cases);
    return 0;
}
'''


def main():
    reference = subprocess.check_output(
        ["git", "show", f"{BASE}:firmware/src/dsp/busfx.h"], cwd=ROOT, text=True
    ).replace("busfx_", "ref_busfx_").replace("BFX_", "REF_BFX_")
    with tempfile.TemporaryDirectory(prefix="busfx-equivalence-") as tmp:
        path = Path(tmp)
        (path / "reference.h").write_text(reference)
        (path / "test.c").write_text(SOURCE)
        for opt in ("-O0", "-O2"):
            subprocess.run(shlex.split(os.environ.get("CC", "cc")) + [
                "-std=c99", opt, "-DX0X_HOST", "-ffp-contract=off", "-fno-fast-math",
                "-I", str(ROOT / "firmware/src/dsp"), str(path / "test.c"),
                "-o", str(path / "test")], check=True)
            subprocess.run([str(path / "test")], check=True)


if __name__ == "__main__":
    main()