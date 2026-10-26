#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Real cq_put source: critical MIDI/audio commands must survive ISR preemption.

The producer replaces a victim slot when a 1024-word ring is full. A simulated
higher-priority audio consumer drains 0, 1, 512, or 1024 entries between victim
selection and publication. Tests cover all four critical event classes and a
wrapping ring. This is a deterministic interleaving test, not a device test.

Default STRICT mode must pass for a corrected engine and currently FAILS against
nightly engine.c at d633833ac369f8466bc2563791614206070e87d1.
Use --baseline-risk to verify the reported defect, including a negative mutant,
without claiming source acceptance. Source-structure changes fail closed and
require a reviewer to update the instrumentation, not a skipped test.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "firmware/src/app/engine.c"
INSERT = "cq[i % CQ] = word;"


def command_queue_source():
    data = SOURCE.read_text(encoding="utf-8")
    start = data.index("enum { C_PARAM_T0 = 0, C_DRUM")
    end = data.index("/* ------------------------------------------------------------ params --- */", start)
    ring = data[start:end]
    if ring.count(INSERT) != 2 or ring.count("#define CQ 1024u") != 1:
        raise ValueError("queue publication changed: review ISR hook and assertions")
    return ring


HARNESS = r'''
static unsigned interrupt_armed, interrupt_count, consumed_by_isr, seen[16];
static void consume(unsigned limit)
{
    uint32_t r = cq_r;
    while (limit-- && r != cq_w) {
        uint32_t type = cq[r % CQ] >> 28;
        if (type < 16u) seen[type]++;
        ++r;
    }
    cq_r = r; /* producer's observed reader position after the audio ISR */
}
static void interrupt_hook(void)
{
    if (!interrupt_armed) return;
    interrupt_armed = 0;
    ++interrupt_count;
    consume(consumed_by_isr);
}
static int one(unsigned type, unsigned drain_count, unsigned offset, int expect_race)
{
    unsigned expected = (expect_race && drain_count) ? 0u : 1u;
    uint32_t i;
    cq_r = cq_w = offset;
    for (i = 0; i < 16u; ++i) seen[i] = 0;
    interrupt_count = interrupt_armed = 0;
    consumed_by_isr = drain_count;
    for (i = 0; i < CQ; ++i) cq_put(C_PARAM_T0, 0, 0, i);
    interrupt_armed = 1;
    cq_put(type, 0, type == C_BRK ? 0u : 60u, 0x345u);
    consume(CQ + 1u);
    if (interrupt_count != 1u || seen[type] != expected || cq_r != cq_w) {
        printf("FAIL event=%u drain=%u offset=%u delivered=%u expected=%u hook=%u\n",
               type, drain_count, offset, seen[type], expected, interrupt_count);
        return 1;
    }
    return 0;
}
int main(void)
{
    const unsigned types[] = {C_DRUM, C_BON, C_BOFF, C_BRK};
    const unsigned drains[] = {0u, 1u, CQ / 2u, CQ};
    const unsigned starts[] = {0u, CQ - 3u};
    unsigned i, j, k, ok = 0;
    for (i = 0; i < sizeof(types)/sizeof(types[0]); ++i)
      for (j = 0; j < sizeof(drains)/sizeof(drains[0]); ++j)
        for (k = 0; k < sizeof(starts)/sizeof(starts[0]); ++k) {
          if (one(types[i], drains[j], starts[k], EXPECT_RACE)) return 1;
          ++ok;
        }
    printf("PASS %u source-derived CQ preemption scenarios (risk_mode=%d)\n", ok, EXPECT_RACE);
    return 0;
}
'''


def run(compiler, ring, expect_race, hook_after, sanitizer):
    replacement = (INSERT + " interrupt_hook();") if hook_after else ("interrupt_hook(); " + INSERT)
    instrumented = ring.replace(INSERT, replacement)
    source = ("#include <stdint.h>\n#include <stdio.h>\n"
              "static void interrupt_hook(void);\n"
              + instrumented + "\n#define EXPECT_RACE " + str(int(expect_race)) + "\n" + HARNESS)
    with tempfile.TemporaryDirectory(prefix="x0x-cq-preemption-") as tmp:
        file = Path(tmp) / "cq.c"
        binary = Path(tmp) / "cq"
        file.write_text(source, encoding="utf-8")
        flags = [compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror"]
        if sanitizer:
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        build = subprocess.run(flags + [str(file), "-o", str(binary)], capture_output=True, text=True)
        if build.returncode:
            print("BUILD FAIL:", build.stderr.strip())
            return 2
        test = subprocess.run([str(binary)], capture_output=True, text=True)
        print(test.stdout.strip() or "TEST FAILED WITHOUT OUTPUT")
        if test.stderr.strip():
            print(test.stderr.strip())
        return test.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-risk", action="store_true", help="confirm existing defect, not a fix")
    parser.add_argument("--compiler", default="cc", help="host C compiler (default cc)")
    parser.add_argument("--sanitize", action="store_true", help="enable ASan/UBSan")
    args = parser.parse_args()
    try:
        ring = command_queue_source()
    except (OSError, ValueError) as error:
        parser.exit(2, f"SOURCE REVIEW REQUIRED: {error}\n")
    rc = run(args.compiler, ring, args.baseline_risk, False, args.sanitize)
    if not args.baseline_risk or rc != 0:
        return rc
    # A mutation moving the preemption AFTER publication must falsify the
    # risk reproduction; otherwise the test might be passing vacuously.
    mutant = run(args.compiler, ring, True, True, args.sanitize)
    if mutant == 0 or mutant == 2:
        print("FAIL negative mutation failed to contradict the baseline race")
        return 3
    print("PASS negative mutation rejected (preempt-after-store)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())