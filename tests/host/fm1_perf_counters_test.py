#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise the target PERF probe and platform toggles with modeled MMIO.

This checks register state and fallback behavior, not hardware clock accuracy.
Run: python3 tests/host/fm1_perf_counters_test.py
"""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + "\n"


def main():
    hal = (ROOT / "firmware/hal/fm1_perf.h").read_text()
    hal = hal.replace('#include "fm1_time.h"', "").replace("#pragma once", "")
    registers = {
        "FM1_C0_IF_UACNTL": ("0x1EEE200u", "fetch"),
        "FM1_C0_RD_UACNTL": ("0x1EEE208u", "reads"),
        "FM1_C0_WR_UACNTL": ("0x1EEE210u", "writes"),
        "FM1_C0_TL_CKCNTL": ("0x1EEE218u", "cycles"),
        "FM1_DBG_CON": ("0x1EEE344u", "debug"),
    }
    for name, (address, variable) in registers.items():
        definition = f"#define {name} (*(volatile uint32_t *){address})"
        if hal.count(definition) != 1:
            raise ValueError(f"review changed MMIO mapping: {name}")
        hal = hal.replace(definition, f"#define {name} {variable}")
    platform = (ROOT / "firmware/src/app/plat_fm1.c").read_text()
    wrappers = "static int perf_cyc_ok, perf_stalls_on;\n" + "".join(
        function(platform, signature) for signature in (
            "void plat_perf_init(void)", "uint32_t plat_cycles(void)",
            "int plat_cycles_cpu(void)", "int plat_stalls(uint32_t s[3])",
            "void plat_stalls_enable(int on)",
        )
    )
    prelude = r"""#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static uint32_t debug, cycles, timer, fetch = 11, reads = 22, writes = 33;
static unsigned ticks, stuck;
#define FM1_TICKS_PER_US 24u
static uint32_t fm1_ticks(void)
{
    assert(++ticks < 200);
    assert((debug & 7u) == 7u); /* Enable must precede the first timed read. */
    timer += 24u;
    if (!stuck)
        cycles += 240u;
    return timer;
}
"""
    harness = r"""
int main(void)
{
    const uint32_t states[] = {0u, 0xA5A5FF00u, 0x80000003u, 0x1234567Fu};
    unsigned cases = 0;
    for (unsigned i = 0; i < sizeof states / sizeof states[0]; ++i)
      for (unsigned wrap = 0; wrap < 2; ++wrap)
        for (stuck = 0; stuck < 2; ++stuck) {
            debug = states[i];
            cycles = timer = wrap ? UINT32_MAX - 48u : 0u;
            ticks = 0;
            plat_perf_init();
            assert(plat_cycles_cpu() == !stuck);
            assert(debug == (stuck ? states[i] : states[i] | 7u));
            if (stuck) {
                debug |= 7u; /* Permit the modeled timer read in fallback. */
                uint32_t prior = timer;
                assert(plat_cycles() == prior + 24u);
                debug = states[i];
            } else {
                assert(plat_cycles() == cycles);
            }
            uint32_t s[3];
            plat_stalls_enable(1);
            assert(plat_stalls(s) == 0);
            assert(s[0] == 11u && s[1] == 22u && s[2] == 33u);
            plat_stalls_enable(0);
            assert(plat_stalls(s) == -1);
            assert(debug == (stuck ? states[i] & ~7u : states[i] | 7u));
            if (!stuck) {
                uint32_t prior = cycles;
                fm1_ticks();
                assert(plat_cycles() - prior == 240u);
            }
            ++cases;
        }
    printf("PASS %u PERF counter cases: enable, fallback, wrap, STALLS continuity\n", cases);
    return 0;
}
"""
    with tempfile.TemporaryDirectory(prefix="x0x-perf-counters-") as tmp:
        source = Path(tmp) / "test.c"
        binary = Path(tmp) / "test"
        source.write_text(prelude + hal + wrappers + harness)
        subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-O2",
                        "-Wall", "-Wextra", "-Werror", str(source),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
