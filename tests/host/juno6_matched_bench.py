#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""One bounded matched timing run; never reruns PCM tests or native builds."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import statistics
import subprocess
import time

import juno6_exact_test as exact

CANDIDATE = "eef741997e743189f17df7e46bf881e472ad254c"
CANDIDATE_SHA256 = "b1e8ab2268a494f469089bb13d6b7bed13b055d8bbb66218617895da6a5422dd"
CASES = ("idle", "held-1", "held-6", "release-1", "release-6")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def interval(values):
    """Exact sign-test median interval with >=99% nominal coverage (iid)."""
    values = sorted(values)
    n = len(values)
    ks = [k for k in range(1, n // 2 + 1)
          if 2 * sum(math.comb(n, j) for j in range(k)) / 2**n <= .01]
    k = max(ks)
    return [values[k - 1], values[n - k]], 1 - 2 * sum(math.comb(n, j) for j in range(k)) / 2**n


def summarize(raw, cpu):
    rows = [json.loads(line) for line in raw.read_text().splitlines()]
    if len(rows) != 5 * 15 * 16 * 2:
        raise ValueError("incomplete timing record")
    if any(any(c != cpu for c in row["cpus"]) for row in rows):
        raise ValueError("CPU affinity changed during measurement")
    identities = {(r["case"], r["trial"], r["quartet"], r["control"]) for r in rows}
    if len(identities) != len(rows):
        raise ValueError("duplicate timing records")
    result = []
    for case in CASES:
        entry = {"case": case}
        for control, label in ((True, "same_function_control"), (False, "candidate_vs_parent")):
            ratios = []
            samples = []
            for trial in range(15):
                group = [r for r in rows if r["case"] == case and r["control"] == control and r["trial"] == trial]
                assert len(group) == 16
                a = b = aw = bw = involuntary = 0
                for r in group:
                    ai = (0, 3) if r["order"] == "ABBA" else (1, 2)
                    bi = (1, 2) if r["order"] == "ABBA" else (0, 3)
                    a += sum(r["cpu_ns"][i] for i in ai)
                    b += sum(r["cpu_ns"][i] for i in bi)
                    aw += sum(r["wall_ns"][i] for i in ai)
                    bw += sum(r["wall_ns"][i] for i in bi)
                    involuntary += sum(r["involuntary"])
                ratios.append(b / a)
                samples.append({"trial": trial, "a_cpu_ns": a, "b_cpu_ns": b,
                                "a_wall_ns": aw, "b_wall_ns": bw,
                                "ratio": b / a, "involuntary_switches": involuntary})
            ci, coverage = interval(ratios)
            entry[label] = {"median_ratio": statistics.median(ratios), "median_interval": ci,
                            "nominal_coverage": coverage, "min_ratio": min(ratios),
                            "max_ratio": max(ratios), "samples": samples}
        ci = entry["same_function_control"]["median_interval"]
        entry["control_qualified"] = .99 <= ci[0] <= 1 <= ci[1] <= 1.01
        entry["classification"] = "INCONCLUSIVE"
        if entry["control_qualified"]:
            lo, hi = entry["candidate_vs_parent"]["median_interval"]
            if lo > 1:
                entry["classification"] = "REGRESSION_SIGNAL_REQUIRES_REVERSED_LAYOUT"
            elif hi <= 1:
                entry["classification"] = "NO_INCREASE_IN_THIS_MEASURED_CASE"
            else:
                entry["classification"] = "OVERLAPS_EQUAL_COST"
        result.append(entry)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cpu", type=int, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    dsp = root / "firmware/src/dsp"
    parent = root / "tests/host/fixtures/juno6-hoist-parent/juno6.c"
    exact.pin(parent, "juno6.c")
    for name in ("juno6.h", "fastmath.h"):
        exact.pin(dsp / name, name)
    if digest(dsp / "juno6.c") != CANDIDATE_SHA256:
        raise SystemExit("candidate source differs from eef741997e74")
    os.sched_setaffinity(0, {args.cpu})
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    if (out / "protocol.json").exists():
        raise SystemExit("output already contains a run; retain it and choose a new directory explicitly")
    work = root / "build/juno-matched-bench"
    work.mkdir(parents=True, exist_ok=True)
    cc = "cc"
    flags = ["-std=c99", "-ffp-contract=off", "-fno-fast-math", "-Wall", "-Wextra", "-I" + str(dsp)]
    protocol = {"candidate": CANDIDATE, "parent": exact.PARENT, "candidate_source_sha256": CANDIDATE_SHA256,
                "compiler": subprocess.check_output([cc, "--version"], text=True).splitlines()[0],
                "compiler_path": subprocess.check_output(["sh", "-c", "command -v cc"], text=True).strip(),
                "platform": platform.platform(), "cpu_affinity": sorted(os.sched_getaffinity(0)),
                "load_average_before": os.getloadavg(), "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "common_flags": flags, "optimizations": ["Os", "O2"], "link_orders": ["AB", "BA"],
                "sample_rate": 44100, "frames_per_block": 256, "blocks_per_leg": 16,
                "quartets_per_trial": 16, "trials_per_case": 15, "cases": CASES,
                "control": "same parent function pointer for both A and B labels, interleaved with actual comparison",
                "order": "alternate ABBA/BAAB and control/comparison first; rotate case order",
                "control_gate": "99% nominal sign-test median interval includes 1 and lies within [0.99, 1.01]; this is a noise gate, not an allowed regression",
                "acceptance": "reject a qualified regression reproduced in both link layouts; otherwise report case intervals. No universal never-slower assertion; no outlier deletion or repeat-until-pass",
                "interval_limit": "nominal iid median interval; temporal dependence or nonstationarity weakens inference",
                "dsp_and_proof_reuse": "production source unchanged; previous PCM/state and native build proofs are not rerun",
                "driver_sha256": digest(Path(__file__).with_suffix(".c")), "runner_sha256": digest(Path(__file__))}
    (out / "protocol.json").write_text(json.dumps(protocol, indent=2) + "\n")
    compiled = {}
    commands = []
    for opt in ("Os", "O2"):
        for name, source, renames in (("parent", parent, [f"-Djuno6_{api}=parent_juno6_{api}" for api in exact.APIS]),
                                      ("candidate", dsp / "juno6.c", [])):
            obj = work / f"{name}-{opt}.o"
            command = [cc, *flags, "-" + opt, *renames, "-c", str(source), "-o", str(obj)]
            subprocess.run(command, check=True); commands.append(command)
        for order in ("AB", "BA"):
            objects = [work / f"{name}-{opt}.o" for name in (("parent", "candidate") if order == "AB" else ("candidate", "parent"))]
            exe = work / f"bench-{opt}-{order}"
            command = [cc, *flags, "-" + opt, str(Path(__file__).with_suffix(".c")), *map(str, objects), "-lm", "-o", str(exe)]
            subprocess.run(command, check=True); commands.append(command)
            compiled[opt, order] = exe
    (out / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    summaries = []
    # Complete one declared run. No concurrent builds or additional timing workers.
    for opt, order in (("Os", "AB"), ("O2", "BA"), ("Os", "BA"), ("O2", "AB")):
        raw = out / f"raw-{opt}-{order}.jsonl"
        print(f"START {opt} {order}", flush=True)
        with raw.open("w") as stream:
            subprocess.run([str(compiled[opt, order])], stdout=stream, check=True)
        rows = summarize(raw, args.cpu)
        summaries.append({"optimization": opt, "link_order": order,
                          "executable_sha256": digest(compiled[opt, order]),
                          "raw_sha256": digest(raw), "cases": rows})
        print(json.dumps({"finished": f"{opt}-{order}", "cases": [{"case": r["case"], "control_qualified": r["control_qualified"], "classification": r["classification"]} for r in rows]}), flush=True)
    result = {"candidate": CANDIDATE, "protocol_sha256": digest(out / "protocol.json"), "runs": summaries,
              "load_average_after": os.getloadavg(), "completed_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())}
    (out / "summary.json").write_text(json.dumps(result, indent=2) + "\n")


if __name__ == "__main__":
    main()
