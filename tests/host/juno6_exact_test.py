#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Focused actual-parent PCM/state comparison; optional paired host timing."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import statistics
import subprocess
import tempfile

PARENT = "1322293e89c1582eecb3d9b7950e14db01722065"
PINS = {
    "juno6.c": ("902f96b57cc761f975c32bfbd1a31ebf5326dd9c", "b9bd8324ee0c430af6976ccac596c17fbfc80a1efd000930b60b2959bf159404"),
    "juno6.h": ("0a2c151ecb75ad226785b347d9f497a00eb8368f", "1ab73dc959d62d6a52d3a3e2ee0404c1597f20ddf947b6585c709b00ba46ced2"),
    "fastmath.h": ("666d8e1325d32a76f7b4b964f2808420d3987104", "fbe052086010b17d8194b9cd3d61d3cb349cc1def3817ae9651a6683d7850b09"),
}
APIS = ("init", "note_on", "note_off", "all_off", "render", "set", "active",
        "chorus_init", "chorus_set", "chorus_process")


def pin(path, name):
    data = path.read_bytes()
    blob = hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()
    sha = hashlib.sha256(data).hexdigest()
    if (blob, sha) != PINS[name]:
        raise SystemExit(f"parent input mismatch: {path}: blob={blob}, sha256={sha}")
    return data


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    ap.add_argument("--baseline", type=Path)
    ap.add_argument("--candidate", type=Path)
    ap.add_argument("--cc", default=os.environ.get("CC", "cc"))
    ap.add_argument("--opt", choices=("Os", "O2", "both"), default="both")
    ap.add_argument("--sanitize", action="store_true")
    ap.add_argument("--bench", action="store_true")
    ap.add_argument("--cpu", type=int, help="pin this process and children to one allowed CPU")
    args = ap.parse_args()
    root = args.root.resolve()
    dsp = root / "firmware/src/dsp"
    baseline = (args.baseline or root / "tests/host/fixtures/juno6-hoist-parent/juno6.c").resolve()
    candidate = (args.candidate or dsp / "juno6.c").resolve()
    if args.sanitize and args.bench:
        ap.error("sanitizer timing is not representative; run separately")
    if args.cpu is not None:
        os.sched_setaffinity(0, {args.cpu})
    parent_bytes = pin(baseline, "juno6.c")
    headers = {name: pin(dsp / name, name) for name in ("juno6.h", "fastmath.h")}
    cc = shlex.split(args.cc)
    compiler = subprocess.check_output([*cc, "--version"], text=True).splitlines()[0]
    print(json.dumps({"kind": "inputs", "parent_commit": PARENT,
        "parent_blob": PINS["juno6.c"][0], "candidate_sha256": hashlib.sha256(candidate.read_bytes()).hexdigest(),
        "compiler": compiler, "host": platform.platform(),
        "affinity": sorted(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else None,
        "sanitize": args.sanitize}), flush=True)
    opts = ("Os", "O2") if args.opt == "both" else (args.opt,)
    with tempfile.TemporaryDirectory(prefix="juno6-exact-") as temp:
        work = Path(temp)
        (work / "parent.c").write_bytes(parent_bytes)
        for name, data in headers.items():
            (work / name).write_bytes(data)
        for opt in opts:
            flags = ["-std=c99", f"-{opt}", "-ffp-contract=off", "-fno-fast-math",
                     "-Wall", "-Wextra", "-I" + str(work)]
            if args.sanitize:
                flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
            objects = []
            for name, source, renames in (("parent", work / "parent.c", [f"-Djuno6_{api}=parent_juno6_{api}" for api in APIS]),
                                         ("candidate", candidate, [])):
                obj = work / f"{name}-{opt}.o"
                subprocess.run([*cc, *flags, *renames, "-c", str(source), "-o", str(obj)], check=True)
                objects.append(str(obj))
            exe = work / ("test-" + opt)
            subprocess.run([*cc, *flags, str(Path(__file__).with_suffix(".c")), *objects, "-lm", "-o", str(exe)], check=True)
            env = dict(os.environ)
            if args.sanitize:
                env.setdefault("ASAN_OPTIONS", "detect_leaks=0")
            proof = subprocess.check_output([str(exe)], text=True, env=env)
            for line in proof.splitlines():
                row = json.loads(line); row["optimization"] = opt; print(json.dumps(row), flush=True)
            if args.bench:
                run = subprocess.check_output([str(exe), "--bench"], text=True, env=env)
                cases = {}
                for line in run.splitlines():
                    row = json.loads(line); row["optimization"] = opt; print(json.dumps(row), flush=True)
                    cases.setdefault(row["case"], []).append(row)
                for case, rows in cases.items():
                    for clock, suffix in (("CLOCK_MONOTONIC", "_ns"), ("CLOCK_THREAD_CPUTIME_ID", "_cpu_ns")):
                        ratios = [sum(r["candidate" + suffix]) / sum(r["parent" + suffix]) for r in rows]
                        print(json.dumps({"kind": "timing_summary", "case": case, "optimization": opt,
                            "clock": clock, "median_paired_ratio": statistics.median(ratios), "min_paired_ratio": min(ratios),
                            "max_paired_ratio": max(ratios), "interpretation": "host timing evidence; no pass/fail or device-cycle claim"}), flush=True)


if __name__ == "__main__":
    main()
