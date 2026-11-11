#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Producer-only exact-source BREAK live reverse/tempo stress regression.

Compares PCM for BRK_FAST_REV_SKIP=0 vs 1, exercising real BREAK renderer code
while varying live reverse, NoteOff, half speed, pitch, transport, tempo, slice,
loop-bank generation and quiet/STOP. No hardware ISR, MIDI wire, or model build.

--peer-fastmath-file accepts ONLY the already-owned PR28 signed-shift repair,
needed to run an ASan/UBSan projection without inheriting PR93's outstanding
negative-exponent undefined behavior. This flag DOES NOT change production Git.
"""
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

PINS = {
    "firmware/src/dsp/breaks.c": "9e1e607e2ddc3028383bfb6ebc85fbc7ac6864c5",
    "firmware/src/dsp/breaks.h": "6cb71d336c2c90fdc60a9594c2356f3a4379c264",
    "firmware/src/dsp/ima_adpcm.h": "9ba16e4fe9f0cb643c95a68d540042ab2581d4de",
    "firmware/src/dsp/fastmath.h": "7d2e41bf70590bafa8af0a7376bb912bf763b098",
    "firmware/src/dsp/x0x_param.h": "f7c9bf22b0606946905790dc20497a2e4b2cb4a6",
    "firmware/src/seq/pattern.h": "9a0703418aae95b14f1b70905cfac916da38d7dd",
}
PR28_REPAIR_BLOB = "a5c26c95cad0d91a548a8db073f59777e36d4e86"
TEST_C = "tests/host/breaks_live_stress_test.c"
SAMPLE_BYTES = 4 * 4 * 320 * 64 * 4


def git_blob(src: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(src)).encode() + b"\0" + src).hexdigest()


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()


def proc(args, path: Path, cwd=None, env=None) -> int:
    result = subprocess.run(args, text=True, capture_output=True, cwd=cwd, env=env, check=False)
    path.write_text("$ " + " ".join(map(str, args)) + "\n"
                    + "exit=" + str(result.returncode) + "\n"
                    + "STDOUT:\n" + result.stdout + "\nSTDERR:\n" + result.stderr)
    return result.returncode


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source-root", required=True, type=Path)
    p.add_argument("--output-dir", required=True, type=Path)
    p.add_argument("--compiler", choices=("gcc", "clang", "both"), default="both")
    p.add_argument("--sanitizer", action="store_true")
    p.add_argument("--peer-fastmath-file", type=Path)
    args = p.parse_args()
    if args.output_dir.exists():
        p.error("output directory already exists; refuse to overwrite evidence")
    if args.sanitizer and args.peer_fastmath_file is None:
        p.error("sanitizer needs exact peer PR28 fastmath: original header has known signed-shift UB")
    if args.peer_fastmath_file is not None and not args.sanitizer:
        p.error("peer header is allowed ONLY for explicit sanitizer projection")
    source = args.source_root.resolve()
    matched = {}
    for name, expected in PINS.items():
        blob = git_blob((source / name).read_bytes())
        if blob != expected:
            raise ValueError(f"wrong Git input {name}: {blob} != {expected}")
        matched[name] = blob
    harness = (source / TEST_C).read_bytes()
    matched[TEST_C] = git_blob(harness)
    peer = None
    if args.peer_fastmath_file:
        peer = args.peer_fastmath_file.read_bytes()
        if git_blob(peer) != PR28_REPAIR_BLOB:
            raise ValueError("peer fastmath bytes do not match existing PR28 Git blob")
    out = args.output_dir.resolve()
    out.mkdir(parents=True)
    result = {
        "stage": "PR93 PRODUCER HOST STRESS; NEVER independent QA, target build or device PASS",
        "source_commit": "61740dbbc666396fbbe41df66effcf758c2ae45a",
        "source_compiled_firmware_tree": "ee863f9cba6cbfb4e471d199ae408c7694acbac5",
        "input_blobs": matched,
        "projection_peer_PR28_header": PR28_REPAIR_BLOB if peer else None,
        "configurations": [], "trace_bytes_per_variant": SAMPLE_BYTES,
        "scenarios": "16 distinct loop-length/rate cases × 320 64-frame callbacks, live reverse/tempo/half-speed/pitch/NoteOff/bank replacement/STOP",
        "model_build": "NOT_RUN", "owner_device": "NOT_RUN", "root_QA": "NOT_RUN",
    }
    with tempfile.TemporaryDirectory(prefix="x0x-pr93-live-stress-") as tmp:
        shadow = Path(tmp)
        for name in PINS:
            target = shadow / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes((source / name).read_bytes())
        (shadow / TEST_C).parent.mkdir(parents=True, exist_ok=True)
        (shadow / TEST_C).write_bytes(harness)
        if peer is not None:
            (shadow / "firmware/src/dsp/fastmath.h").write_bytes(peer)
        pairs = {}
        for cc in ("gcc", "clang") if args.compiler == "both" else (args.compiler,):
            for flag, label in ((0, "legacy"), (1, "fast")):
                exe = shadow / (cc + "-" + label)
                common = [cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
                          "-ffp-contract=off", "-I" + str(shadow),
                          "-DBRK_FAST_REV_SKIP=" + str(flag)]
                if args.sanitizer:
                    common += ["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"]
                else:
                    common += ["-O2"]
                compile_cmd = common + [str(shadow / TEST_C),
                                        str(shadow / "firmware/src/dsp/breaks.c"), "-o", str(exe)]
                compile_log = out / (cc + "-" + label + "-compile.log")
                status = proc(compile_cmd, compile_log)
                if status:
                    raise RuntimeError(f"compile exit {status}: {compile_log.name}")
                trace = out / (cc + "-" + label + ".pcm")
                run_log = out / (cc + "-" + label + "-run.log")
                env = os.environ.copy()
                if args.sanitizer:
                    env.update(ASAN_OPTIONS="detect_leaks=1:abort_on_error=1",
                               UBSAN_OPTIONS="halt_on_error=1")
                status = proc([str(exe), str(trace)], run_log, env=env)
                if status:
                    raise RuntimeError(f"run exit {status}: {run_log.name}")
                if trace.stat().st_size != SAMPLE_BYTES:
                    raise RuntimeError(f"truncated/extra PCM: {trace.stat().st_size}")
                lines = [x for x in run_log.read_text().splitlines() if x.startswith("n=")]
                if len(lines) != 16 or len(set(lines)) != 16:
                    raise RuntimeError(f"bad scenario rows: {len(lines)}")
                if any(" silent=1" not in x or " aud=0 " in x for x in lines):
                    raise RuntimeError("incomplete stop/audible evidence")
                pairs.setdefault(cc, {})[label] = sha256(trace)
                neglog = out / (cc + "-" + label + "-invalid-command.log")
                if proc([str(exe)], neglog) != 2:
                    raise RuntimeError("missing-argument negative was accepted")
                result["configurations"].append({"compiler": cc, "branch": label,
                                                  "compile_exit": 0, "run_exit": 0,
                                                  "invalid_argument_exit": 2,
                                                  "pcm_sha256": sha256(trace),
                                                  "compile_log_sha256": sha256(compile_log),
                                                  "run_log_sha256": sha256(run_log)})
                trace.unlink()  # maintain hashes only; never publish samples
            if pairs[cc]["legacy"] != pairs[cc]["fast"]:
                raise RuntimeError("non-identical PCM at " + cc)
        if len(pairs) > 1:
            if len(set(x["fast"] for x in pairs.values())) != 1:
                raise RuntimeError("compiler-dependent PCM mismatch")
        result["pcm_sha256_by_compiler"] = pairs
        result["outcome"] = "PASS producer-only PCM/STOP; no hardware or independent QA"
        (out / "manifest.json").write_text(json.dumps(result, sort_keys=True, indent=2) + "\n")
    print(result["outcome"])
    print(json.dumps(pairs, sort_keys=True))

if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        sys.exit(1)
