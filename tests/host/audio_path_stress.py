#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Repeatable host stress; host time is NOT target ISR proof.
Use an unchanged compiler/asset/flag configuration for before/after captures.
  python3 tests/host/audio_path_stress.py --output DIR --fixture FILE.wav
Add --all-presets to sweep each Chicago/Miami preset against a paired program.
Baseline overrides are source files, not copies of the DSP algorithm.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--output", type=Path, required=True)
    ap.add_argument("--fixture", type=Path, required=True)
    ap.add_argument("--baseline-engine", type=Path)
    ap.add_argument("--baseline-master", type=Path)
    ap.add_argument("--reference", type=Path)
    ap.add_argument("--all-presets", action="store_true")
    ap.add_argument("--blocks", type=int, default=1024)
    ap.add_argument("--enforce-headroom", action="store_true",
                    help="Fail if the measured host peak exceeds 60 percent, or either synth exceeds 15 percent")
    args = ap.parse_args()
    root = Path(__file__).resolve().parents[2]
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    subprocess.run(["sh", "host/build_host.sh", "--generate-only"], cwd=root,
                   stdout=subprocess.DEVNULL, check=True)
    setup = subprocess.check_output(
        ["sh", "-c", '. tests/host/original_build.sh; printf "%s\\n" "$ORIGINAL_FLAGS" "$ORIGINAL_SOURCES"'],
        cwd=root, text=True).splitlines()
    if len(setup) != 2:
        raise RuntimeError("unexpected production source/flag declaration")
    flags, sources = map(shlex.split, setup)
    replacements = {"firmware/src/app/engine.c": args.baseline_engine,
                    "firmware/src/dsp/master.c": args.baseline_master}
    sources = [str(replacements[s].resolve()) if replacements.get(s) else s for s in sources]
    binary = out / "stress"
    command = [os.environ.get("CC", "cc"), "-O2", "-ffp-contract=off", "-std=c99",
               "-Wno-unused-function", "-Wno-unused-parameter", "-DX0X_HOST",
               "-iquote", str(root / "firmware/src/dsp"),
               *flags, *shlex.split(os.environ.get("X0X_HOST_DEFS", "")),
               "-Ifirmware/src", "-Ifirmware/src/app", "-Ifirmware/src/dsp", "-Ibuild/gen",
               "tests/host/audio_path_stress.c", *sources, "-lm", "-o", str(binary)]
    with (out / "build.log").open("w") as log:
        subprocess.run(command, cwd=root, stdout=log, stderr=subprocess.STDOUT, check=True)
    pairs = ([(c, c % 30) for c in range(55)] + [(0, m) for m in range(30)]
             if args.all_presets else [(0, 0), (27, 15), (54, 29)])
    results = []
    hashes = {}
    for case, (chicago, miami) in enumerate(pairs):
        for sweep in (0, 1):
            label = f"{case:03d}-c{chicago}-m{miami}-s{sweep}"
            directory = out / label
            directory.mkdir(exist_ok=True)
            process = subprocess.run([str(binary), str(directory), str(args.fixture.resolve()),
                                      str(chicago), str(miami), str(args.blocks), str(sweep)],
                                     cwd=root, text=True, capture_output=True, check=True)
            row = json.loads(process.stdout.splitlines()[-1])
            row["case"] = label
            results.append(row)
            for file in sorted(directory.iterdir()):
                hashes[f"{label}/{file.name}"] = {
                    "sha256": hashlib.sha256(file.read_bytes()).hexdigest(),
                    "bytes": file.stat().st_size}
            print(label, "peak_pct", row["peak_pct"], flush=True)
    summary = {
        "kind": "HOST SIMULATION: engine + USB volume; excludes target HAL and interrupt preemption",
        "budget_ms": 11.6, "target_peak_ms": 6.96, "synth_target_peak_ms": 1.74,
        "configured_voices": {"chicago": 1, "miami_unison_lanes": 2},
        "cases": len(results), "blocks": sum(r["blocks"] for r in results),
        "peak_ns": max(r["peak_ns"] for r in results),
        "average_ns": sum(r["average_ns"] * r["blocks"] for r in results) / sum(r["blocks"] for r in results),
        "chicago_peak_ns": max(r["chicago_peak_ns"] for r in results),
        "miami_peak_ns": max(r["miami_peak_ns"] for r in results),
        "deadline_misses": sum(r["deadline_misses"] for r in results),
        "blocks_above_60pct": sum(r["blocks_above_60pct"] for r in results),
        "foreground_peak_ns": max(r["foreground_peak_ns"] for r in results),
    }
    summary["margin_ns"] = 11600000 - summary["peak_ns"]
    summary["host_headroom_pass"] = (summary["peak_ns"] <= 6960000 and
                                     summary["chicago_peak_ns"] <= 1740000 and
                                     summary["miami_peak_ns"] <= 1740000)
    (out / "results.json").write_text(json.dumps({"summary": summary, "cases": results}, indent=2) + "\n")
    (out / "hashes.json").write_text(json.dumps(hashes, indent=2) + "\n")
    print(json.dumps(summary, indent=2))
    if args.reference:
        expected = json.loads((args.reference / "hashes.json").read_text())
        changed = sorted(k for k in expected.keys() | hashes.keys() if expected.get(k) != hashes.get(k))
        if changed:
            print("FAIL: stress audio/state mismatch: " + ", ".join(changed), file=sys.stderr)
            return 1
        print(f"PASS: {len(hashes)} stress audio/trace files bit-identical")
    return 1 if args.enforce_headroom and not summary["host_headroom_pass"] else 0

if __name__ == "__main__":
    raise SystemExit(main())
