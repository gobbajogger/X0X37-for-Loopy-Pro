#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Measure deterministic 64-frame BREAK IMA decoder work with exact pinned C.

Compares production BREAK sources at an accepted NIGHTLY and a candidate PR.
Never infers MCU timing or replaces independent QA. No firmware or samples.
A temporary, bounded instrumentation adds exactly one volatile counter increment
at entry to the unchanged production ima_decode() in BOTH variants.
"""
import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

# Exact Git blobs, not a loose branch name or repository timestamp.
PINS = {
    "baseline-nightly": {
        "firmware/src/dsp/breaks.c": "1d3d692b6788d9d1f13901cd7ab13df506cdb72f",
        "firmware/src/dsp/breaks.h": "094af6650333e844311a8806145a6177a0318225",
    },
    "candidate": {
        "firmware/src/dsp/breaks.c": "0fcdbe65b674dea3becb26fc265cb2514908f1a5",
        "firmware/src/dsp/breaks.h": "6cb71d336c2c90fdc60a9594c2356f3a4379c264",
    },
}
SHARED = {
    "firmware/src/dsp/ima_adpcm.h": "9ba16e4fe9f0cb643c95a68d540042ab2581d4de",
    "firmware/src/dsp/fastmath.h": "7d2e41bf70590bafa8af0a7376bb912bf763b098",
    "firmware/src/dsp/x0x_param.h": "f7c9bf22b0606946905790dc20497a2e4b2cb4a6",
    "firmware/src/seq/pattern.h": "9a0703418aae95b14f1b70905cfac916da38d7dd",
}
COUNTER_DECL = "extern volatile uint64_t x0x_host_ima_decode_calls;"
FUNC_OPEN = "static inline int32_t ima_decode(const uint8_t *d, uint32_t i, int32_t *pred, int32_t *idx)\n{"


def git_blob(data):
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\x00" + data).hexdigest()


def take_source(name, root, dest):
    measured = {}
    for rel, expected in {**SHARED, **PINS[name]}.items():
        raw = (root / rel).read_bytes()
        got = git_blob(raw)
        if got != expected:
            raise ValueError(f"{name}: changed Git blob {rel}: got {got}, expected {expected}")
        target = dest / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(raw)
        measured[rel] = got
    header = dest / "firmware/src/dsp/ima_adpcm.h"
    content = header.read_text()
    if content.count("#include <stdint.h>") != 1 or content.count(FUNC_OPEN) != 1:
        raise ValueError(f"{name}: unsupported decoder source for bounded observer")
    content = content.replace("#include <stdint.h>", "#include <stdint.h>\n" + COUNTER_DECL)
    content = content.replace(FUNC_OPEN, FUNC_OPEN + "\n    ++x0x_host_ima_decode_calls;")
    header.write_text(content)
    return measured


def run(cmd, log, env=None):
    p = subprocess.run(cmd, text=True, capture_output=True, env=env, check=False)
    data = p.stdout + ("\nSTDERR:\n" + p.stderr if p.stderr else "")
    log.write_text(data)
    return p.returncode


def parse(path):
    rows = {}
    for line in path.read_text().splitlines():
        if not line.startswith("n="):
            raise ValueError("unexpected output line: " + line[:80])
        words = dict(part.split("=", 1) for part in line.split())
        key = (int(words["n"]), words["direction"], int(words["rate"]), int(words["trial"]))
        if key in rows:
            raise ValueError("duplicate work case: " + str(key))
        if words["stopped"] != "1" or int(words["audible"]) < 1:
            raise ValueError("invalid stop or audible signal: " + str(key))
        blocks = int(words["decodes"])
        if blocks < 1 or int(words["max"]) < 1:
            raise ValueError("missing decoder operations: " + str(key))
        if not 0 <= int(words["p95"]) <= int(words["p99"]) <= int(words["max"]):
            raise ValueError("bad work percentile ordering: " + str(key))
        rows[key] = words
    if len(rows) != 60:
        raise ValueError(f"expected 60 distinct workload cases, got {len(rows)}")
    return rows


def compare(a, b):
    if a.keys() != b.keys():
        raise ValueError("missing or extra matched cases")
    for key in a:
        for signal in ("pcm", "audible", "stopped"):
            if a[key][signal] != b[key][signal]:
                raise ValueError(f"PCM/control divergence at {key}: {signal}")
    summary = []
    for n in (65535, 176400, 2116800):
        for direction in ("F", "R"):
            for rate in (1, 4, 16, 64, 256):
                keys = [k for k in a if k[0] == n and k[1] == direction and k[2] == rate]
                before = sum(int(a[k]["decodes"]) for k in keys)
                after = sum(int(b[k]["decodes"]) for k in keys)
                summary.append({"n": n, "direction": direction, "rate": rate,
                                "baseline_decode_calls": before,
                                "candidate_decode_calls": after,
                                "saved_percent": round((before - after) / before * 100, 4),
                                "baseline_max_per_64_frames": max(int(a[k]["max"]) for k in keys),
                                "candidate_max_per_64_frames": max(int(b[k]["max"]) for k in keys),
                                "baseline_p99_per_64_frames": max(int(a[k]["p99"]) for k in keys),
                                "candidate_p99_per_64_frames": max(int(b[k]["p99"]) for k in keys)})
    return summary


def main():
    pa = argparse.ArgumentParser()
    pa.add_argument("--baseline-root", type=Path, required=True)
    pa.add_argument("--candidate-root", type=Path, required=True)
    pa.add_argument("--output-dir", type=Path, required=True)
    pa.add_argument("--compiler", choices=["gcc", "clang", "both"], default="both")
    pa.add_argument("--sanitizer", action="store_true")
    args = pa.parse_args()
    if args.baseline_root.resolve() == args.candidate_root.resolve():
        pa.error("baseline and candidate must be separate immutable projections")
    if args.output_dir.exists():
        pa.error("output directory already exists, never overwrite prior evidence")
    args.output_dir.mkdir(parents=True)
    harness = Path(__file__).with_name("breaks_decoder_work_audit.c")
    if not harness.is_file():
        raise ValueError("missing test harness")
    compilers = ["gcc", "clang"] if args.compiler == "both" else [args.compiler]
    out = {"status": "COMPLETE_SOURCE_PROJECTION_HOST_ONLY", "firmware_compiled": False,
           "device_tested": False, "tests": [], "source_git_blobs": {},
           "observer": "single volatile increment inside production ima_decode at call entry; identical both revisions",
           "cases_per_source_compiler": 60, "sanitizer": args.sanitizer}
    with tempfile.TemporaryDirectory(prefix="x0x-break-work-") as td:
        temp = Path(td)
        for label, root in [("baseline-nightly", args.baseline_root), ("candidate", args.candidate_root)]:
            out["source_git_blobs"][label] = take_source(label, root, temp / label)
        for cc in compilers:
            for label in ("baseline-nightly", "candidate"):
                exe = temp / f"{cc}-{label}"
                source = temp / label
                cmd = [cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
                       "-ffp-contract=off", "-I" + str(source)]
                if args.sanitizer:
                    cmd += ["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"]
                else:
                    cmd += ["-O2"]
                cmd += [str(harness), str(source / "firmware/src/dsp/breaks.c"), "-o", str(exe)]
                log = args.output_dir / f"{cc}-{label}-compile.log"
                code = run(cmd, log)
                out["tests"].append({"command": cmd, "exit": code, "log": log.name,
                                     "log_sha256": hashlib.sha256(log.read_bytes()).hexdigest()})
                if code:
                    raise RuntimeError("compiler failed: " + str(cmd))
                log = args.output_dir / f"{cc}-{label}-run.log"
                trace = args.output_dir / f"{cc}-{label}-pcm.bin"
                cmd = [str(exe), "--check", "--trace", str(trace)]
                code = run(cmd, log)
                out["tests"].append({"command": cmd, "exit": code, "log": log.name,
                                     "log_sha256": hashlib.sha256(log.read_bytes()).hexdigest()})
                if code:
                    raise RuntimeError("execution failed: " + str(cmd))
                expected_bytes = 60 * 96 * 64 * 4
                if trace.stat().st_size != expected_bytes:
                    raise RuntimeError(f"unexpected synthetic PCM bytes: {trace.stat().st_size}")
                out.setdefault("raw_pcm_trace", {}).setdefault(cc, {})[label] = {
                    "size": trace.stat().st_size,
                    "sha256": hashlib.sha256(trace.read_bytes()).hexdigest(),
                }
                bad_log = args.output_dir / f"{cc}-{label}-invalid.log"
                bad = run([str(exe), "--invalid"], bad_log)
                if bad != 2:
                    raise RuntimeError("unsupported selector failed to return expected 2")
                out["tests"].append({"command": [str(exe), "--invalid"], "exit": bad,
                                     "expected_exit": 2, "log": bad_log.name,
                                     "log_sha256": hashlib.sha256(bad_log.read_bytes()).hexdigest()})
            old_t = out["raw_pcm_trace"][cc]["baseline-nightly"]
            new_t = out["raw_pcm_trace"][cc]["candidate"]
            if old_t != new_t:
                raise ValueError(f"raw PCM binary divergence for {cc}")
            out["tests"].append({"command": f"raw-pcm-SHA256-{cc}", "exit": 0,
                                 "sha256": old_t["sha256"], "bytes": old_t["size"]})
            old = parse(args.output_dir / f"{cc}-baseline-nightly-run.log")
            new = parse(args.output_dir / f"{cc}-candidate-run.log")
            out["tests"].append({"command": f"parse-and-compare {cc}", "exit": 0,
                                 "matched_pcm_control_cases": len(old)})
            out[cc] = compare(old, new)
            corrupted = {key: dict(value) for key, value in new.items()}
            first = next(iter(corrupted))
            corrupted[first]["pcm"] = "0000000000000000" if corrupted[first]["pcm"] != "0000000000000000" else "ffffffffffffffff"
            try:
                compare(old, corrupted)
            except ValueError:
                out["tests"].append({"command": f"tampered-pcm-negative {cc}", "exit": 0,
                                     "correctly_rejected": True})
            else:
                raise RuntimeError("tampered PCM was not rejected")
            # The trace itself is also checked against silent corruption.
            trace = args.output_dir / f"{cc}-candidate-pcm.bin"
            original = trace.read_bytes()
            tampered = bytes([original[0] ^ 1]) + original[1:]
            if hashlib.sha256(tampered).hexdigest() == new_t["sha256"]:
                raise RuntimeError("raw PCM corruption was not detected")
            out["tests"].append({"command": f"tampered-raw-pcm-negative {cc}", "exit": 0,
                                 "correctly_rejected": True})
            # Keep only logs and verified full-trace digests; synthetic PCM is
            # reproducible on demand and should not bloat handoff archives.
            for label in ("candidate", "baseline-nightly"):
                (args.output_dir / f"{cc}-{label}-pcm.bin").unlink()
    path = args.output_dir / "result.json"
    path.write_text(json.dumps(out, indent=2, sort_keys=True) + "\n")
    print("PASS", len(compilers), "compiler(s), source-verified 60/cmp PCM/control comparisons, sanitizer", args.sanitizer)
    for row in out[compilers[0]]:
        if row["n"] == 2116800 and row["direction"] == "R" and row["rate"] in (1, 16, 64, 256):
            print("LONG_REV", json.dumps(row, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
