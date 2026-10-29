#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Focused canonical-display observers; no original private data or hardware.

Links the actual sequencer, FX bus and master. An optional baseline source tree
is built with the identical fixture and flags; complete event, normalized
sequencer-state and float PCM artifacts must match. Engine observer hooks are
source-extracted, not presented as a whole-engine integration test.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def run(cmd, log, env=None):
    with Path(log).open("w") as stream:
        stream.write("COMMAND " + shlex.join(map(str, cmd)) + "\n")
        stream.flush()
        result = subprocess.run(list(map(str, cmd)), stdout=stream,
                                stderr=subprocess.STDOUT, env=env)
    if result.returncode:
        raise RuntimeError(f"exit {result.returncode}: {log}")


def one(pattern, source):
    matches = re.findall(pattern, source, re.S)
    if len(matches) != 1:
        raise AssertionError(f"expected exactly one source fragment: {pattern}")
    return matches[0]


def engine_probe(root, out):
    source = (root / "firmware/src/app/engine.c").read_text()
    getter = one(r"(uint8_t engine_tape_active\(void\)\n\{.*?\n\})", source)
    master = one(r"(int engine_master_get\(int i\) \{[^\n]+\})", source)
    publication = one(r"fxbus_process_st\(&fx,[^;]+;\n"
                      r"(        if \(fx\.dl_type.*?\n        else if[^\n]+)", source)
    hold = one(r"if \(!n\) return;\n(    if \(project_hold\) \{.*?\n    \})", source)
    init = one(r"(    fxbus_init\(&fx, dly_buf, DLY_LEN\);\n    tape_signal_seen = 0;)", source)
    # Preserve source fragments, including the getter's read-before-clock order.
    text = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "dsp/master.h"
#include "dsp/fxbus.h"
static volatile uint8_t tape_signal_seen;
static volatile uint32_t tape_signal_at;
static volatile uint8_t project_hold;
static uint32_t clock_ms;
static int interrupt_on_clock_read;
static master_t mst;
static fxbus_t fx;
enum { DLY_LEN = 79200 };
static int16_t dly_buf[DLY_LEN];
static uint32_t plat_ms(void)
{
    uint32_t sampled = clock_ms;
    if (interrupt_on_clock_read) {
        tape_signal_at = ++clock_ms;
        tape_signal_seen = 1;
    }
    return sampled;
}
'''
    text += getter + "\n" + master + "\nstatic void publish(void)\n{\n" + publication + "\n}\n"
    text += "static void hold_render(int16_t *out_lr, uint32_t n)\n{\n    uint32_t i;\n" + hold + "\n}\n"
    text += "static void init_fx(void)\n{\n" + init + "\n}\n"
    text += r'''
int main(void)
{
    unsigned checks = 0;
    unsigned char before[sizeof mst];
    int16_t output[4] = {1, 2, 3, 4};
#define CHECK(c) do { assert(c); checks++; } while (0)
    tape_signal_seen = 1; tape_signal_at = 999; init_fx();
    CHECK(!engine_tape_active());
    fx.dl_type = 1; fx.dl_level = 1.0f; fx.dl_signal = 1; fx.muted = 0;
    clock_ms = 100; publish(); CHECK(tape_signal_at == 100 && engine_tape_active());
    fx.dl_signal = 0;
    clock_ms = 179; publish(); CHECK(engine_tape_active());
    clock_ms = 180; publish(); CHECK(!engine_tape_active());
    fx.dl_signal = 1; clock_ms = 200; publish(); CHECK(engine_tape_active());
    fx.dl_type = 0; publish(); CHECK(!engine_tape_active());
    fx.dl_type = 1; publish(); CHECK(engine_tape_active());
    fx.dl_level = 0; publish(); CHECK(!engine_tape_active());
    fx.dl_level = 1.0f; publish(); CHECK(engine_tape_active());
    fx.muted = 2; publish(); CHECK(!engine_tape_active());
    fx.muted = 0; publish(); CHECK(engine_tape_active());
    project_hold = 1; hold_render(output, 2);
    CHECK(!engine_tape_active());
    CHECK(output[0] == 0 && output[1] == 0 && output[2] == 0 && output[3] == 0);
    tape_signal_seen = 1; tape_signal_at = UINT32_MAX - 39u; clock_ms = 39;
    CHECK(engine_tape_active());
    clock_ms = 40; CHECK(!engine_tape_active());
    clock_ms = 100; tape_signal_at = 100; interrupt_on_clock_read = 1;
    CHECK(engine_tape_active());
    CHECK(clock_ms == 101 && tape_signal_at == 101);
    interrupt_on_clock_read = 0;
    master_init(&mst);
    for (int i = 0; i < MST_NPARAMS; i++) {
        master_set(&mst, i, master_param(i)->max / 2);
        memcpy(before, &mst, sizeof mst);
        CHECK(engine_master_get(i) == mst.pot[i]);
        CHECK(memcmp(before, &mst, sizeof mst) == 0);
    }
    CHECK(engine_master_get(-1) == 0 && engine_master_get(MST_NPARAMS) == 0);
    printf("PASS engine_observer_source_hooks checks=%u\n", checks);
    return 0;
}
'''
    path = out / "engine-observer-source-probe.c"
    path.write_text(text)
    return path, {name: hashlib.sha256(value.encode()).hexdigest() for name, value in
                  {"getter": getter, "master": master, "publication": publication,
                   "project_hold": hold, "fx_init": init}.items()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("out", type=Path)
    parser.add_argument("--baseline-source", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    if (out / "RECEIPT.json").exists():
        raise RuntimeError("refusing to overwrite an existing completed receipt")
    compiler = shlex.split(os.environ.get("CC", "cc"))
    common = ["-std=c99", "-Os", "-fno-builtin", "-U_FORTIFY_SOURCE", "-D_FORTIFY_SOURCE=0",
              "-fno-fast-math", "-ffp-contract=off", "-Wall", "-Wextra", "-Wno-attributes",
              "-Wno-unused-function", "-DX0X_HOST", "-DX0X_ORIGINAL_SYNTH=1",
              "-DX0X_DEV_WOBBLE=0", "-DX0X_WOBBLE=0"]
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
    gen = out / "gen"
    gen.mkdir(exist_ok=True)
    run([sys.executable, root / "tools/gen_drum_samples.py", gen / "x0x_drum_samples.h"], out / "generate.log")
    ctest = root / "tests/host/canonical_observers_test.c"
    probe, fragments = engine_probe(root, out)
    units = ["seq/sequencer.c", "seq/pattern.c", "seq/tb3po.c", "dsp/fxbus.c",
             "dsp/x0x_param.c", "dsp/drum909.c"]
    cases = []
    sources = [("candidate", root, 1)]
    if args.baseline_source:
        sources.append(("baseline", args.baseline_source.resolve(), 0))
    for mode in ("normal", "sanitized"):
        extra = [] if mode == "normal" else ["-g", "-fsanitize=address,undefined",
                                              "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
        for name, src, expected in sources:
            directory = out / (name + "-" + mode)
            directory.mkdir(exist_ok=True)
            includes = ["-I" + str(src / "firmware/src"), "-I" + str(src / "firmware/src/dsp"), "-I" + str(gen)]
            executable = directory / "observers"
            command = compiler + common + extra + includes + ["-DEXPECT_OBSERVERS=" + str(expected), str(ctest)]
            command += [str(src / "firmware/src" / unit) for unit in units]
            command += ["-lm", "-o", str(executable)]
            run(command, directory / "compile.log")
            paths = [directory / filename for filename in ("events.bin", "states.bin", "pcm-f32.bin")]
            run([executable, *paths], directory / "run.log", env)
            cases.append({"name": name, "mode": mode, "source": str(src), "command": command,
                          "artifacts": {p.name: {"bytes": p.stat().st_size, "sha256": sha(p)} for p in paths},
                          "run_log_sha256": sha(directory / "run.log")})
        includes = ["-I" + str(root / "firmware/src"), "-I" + str(root / "firmware/src/dsp"), "-I" + str(gen)]
        exe = out / ("engine-probe-" + mode)
        run(compiler + common + extra + includes + [probe, root / "firmware/src/dsp/master.c",
            root / "firmware/src/dsp/fxbus.c", root / "firmware/src/dsp/drum909.c",
            root / "firmware/src/dsp/x0x_param.c", "-lm", "-o", exe], out / ("engine-" + mode + ".compile.log"))
        run([exe], out / ("engine-" + mode + ".log"), env)
    first = cases[0]["artifacts"]
    if any(case["artifacts"] != first for case in cases):
        raise AssertionError("event/state/PCM mismatch; artifacts retained")
    pinned = ["firmware/src/" + unit for unit in units] + ["firmware/src/app/engine.c",
              "firmware/src/app/engine.h", "firmware/src/seq/sequencer.h", "firmware/src/dsp/fxbus.h",
              "firmware/src/dsp/master.c", "firmware/src/dsp/master.h", "firmware/src/dsp/drum909_dsp.h",
              "firmware/src/dsp/fastmath.h", "tools/gen_drum_samples.py",
              "tests/host/canonical_observers_test.c", "tests/host/canonical_observers_test.py"]
    receipt = {"result": "PASS", "baseline_compared": bool(args.baseline_source),
               "scope": "actual sequencer and FX; source-extracted engine hooks; no whole-engine or target timing claim",
               "compiler": subprocess.check_output(compiler + ["--version"], text=True).splitlines()[0],
               "flags": common, "ASAN_OPTIONS": env["ASAN_OPTIONS"], "cases": cases,
               "engine_source_fragments": fragments,
               "inputs": [{"path": name, "sha256": sha(root / name)} for name in pinned],
               "generated": {p.name: sha(p) for p in sorted(gen.glob("*.h"))}}
    (out / "RECEIPT.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print("PASS canonical observer normal/sanitized checks; complete event/state/PCM identity" +
          (" including baseline" if args.baseline_source else ""))
    print(out / "RECEIPT.json")


if __name__ == "__main__":
    main()
