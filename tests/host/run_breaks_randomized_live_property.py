#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exact-source producer-only seeded BREAK two-bank / live NoteOff A/B regression.

Checks bit-identical production renderer PCM under unaltered production compiler
inputs with BRK_FAST_REV_SKIP=0 and =1. The sanitizer path is a clearly labeled
projection of the separately owned PR #28 fix; original PR #93 UBSan is FAIL.
This neither independently certifies firmware nor replaces an Elite/Pro build.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

SOURCE_BLOBS = {
    "firmware/src/dsp/breaks.c": "9e1e607e2ddc3028383bfb6ebc85fbc7ac6864c5",
    "firmware/src/dsp/breaks.h": "6cb71d336c2c90fdc60a9594c2356f3a4379c264",
    "firmware/src/dsp/ima_adpcm.h": "9ba16e4fe9f0cb643c95a68d540042ab2581d4de",
    "firmware/src/dsp/fastmath.h": "7d2e41bf70590bafa8af0a7376bb912bf763b098",
    "firmware/src/dsp/x0x_param.h": "f7c9bf22b0606946905790dc20497a2e4b2cb4a6",
    "firmware/src/seq/pattern.h": "9a0703418aae95b14f1b70905cfac916da38d7dd",
}
TEST_C = "tests/host/breaks_randomized_live_property_test.c"
PEER_PR28_HEADER_BLOB = "a5c26c95cad0d91a548a8db073f59777e36d4e86"
EXPECTED_PCM_BYTES = 7 * 4 * (160 + 16) * 64 * 4
EXPECTED_CASES = 7 * 4


def git_blob(buf: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(buf)).encode() + b"\0" + buf).hexdigest()


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def command(argv, logfile: Path, *, env=None) -> int:
    completed = subprocess.run(argv, text=True, capture_output=True, check=False,
                               env=env, timeout=120)
    logfile.write_text("$ " + " ".join(map(str, argv)) + "\n" +
                       "exit=" + str(completed.returncode) + "\n" +
                       "STDOUT:\n" + completed.stdout + "\nSTDERR:\n" + completed.stderr)
    return completed.returncode


def assert_equal(path_a: Path, path_b: Path):
    if sha256(path_a) != sha256(path_b):
        raise RuntimeError("legacy and optimized PCM are not bit-identical")
    if path_a.stat().st_size != path_b.stat().st_size:
        raise RuntimeError("PCM trace lengths differ")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--compiler", choices=("gcc", "clang", "both"), default="both")
    parser.add_argument("--sanitizer", action="store_true")
    parser.add_argument("--peer-fastmath-file", type=Path)
    args = parser.parse_args()
    if args.output_dir.exists():
        parser.error("output path already exists; evidence overwrite refused")
    if args.sanitizer != (args.peer_fastmath_file is not None):
        parser.error("sanitizer requires ONLY the separately verified PR28 peer-header projection")
    root = args.source_root.resolve()
    pins = {}
    for path, expected in SOURCE_BLOBS.items():
        actual = git_blob((root / path).read_bytes())
        if actual != expected:
            raise RuntimeError(f"wrong source Git blob {path}: {actual} != {expected}")
        pins[path] = actual
    harness = (root / TEST_C).read_bytes()
    pins[TEST_C] = git_blob(harness)
    peer = None
    if args.peer_fastmath_file is not None:
        peer = args.peer_fastmath_file.read_bytes()
        if git_blob(peer) != PEER_PR28_HEADER_BLOB:
            raise RuntimeError("wrong PR28 peer fastmath source blob")
    dest = args.output_dir.resolve()
    dest.mkdir(parents=True, exist_ok=False)
    report = {
        "role": "Performance Engineering producer (never Independent QA)",
        "source_pr": 93,
        "consumed_source_sha": "7dfcf9b2e4b9925628a569dd18cf873385f22618",
        "compiled_firmware_tree": "ee863f9cba6cbfb4e471d199ae408c7694acbac5",
        "source_git_blobs": pins,
        "sanitizer_peer_projection_blob": PEER_PR28_HEADER_BLOB if peer else None,
        "original_source_ubsan": "KNOWN_FAIL: fastmath.h:57 negative signed shift, owned by PR28",
        "target_build": "NOT_RUN", "independent_qa": "NOT_RUN", "hardware": "NOT_RUN",
        "case_count_per_executable": EXPECTED_CASES,
        "expected_pcm_bytes_per_executable": EXPECTED_PCM_BYTES,
        "scope": ("seeded two-bank replacement, overlapping live NoteOn/NoteOff, "
                  "reverse/half/stutter, pitched fractional tempo, retrigger, slice pin, "
                  "64-frame callback canaries and STOP"),
        "checks": [],
    }
    with tempfile.TemporaryDirectory(prefix="x0x-break-live-prop-") as tmp:
        shadow = Path(tmp)
        for path in SOURCE_BLOBS:
            (shadow / path).parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(root / path, shadow / path)
        (shadow / TEST_C).parent.mkdir(parents=True, exist_ok=True)
        (shadow / TEST_C).write_bytes(harness)
        if peer is not None:
            (shadow / "firmware/src/dsp/fastmath.h").write_bytes(peer)
        all_hashes = []
        for cc in (("gcc", "clang") if args.compiler == "both" else (args.compiler,)):
            pcm_pair = {}
            for fast, name in ((0, "legacy"), (1, "fast")):
                ident = cc + "-" + name
                out_bin = shadow / ident
                argv = [cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
                        "-ffp-contract=off", "-I" + str(shadow),
                        "-DBRK_FAST_REV_SKIP=" + str(fast)]
                if args.sanitizer:
                    argv += ["-O1", "-g", "-fno-omit-frame-pointer",
                             "-fsanitize=address,undefined"]
                else:
                    argv += ["-O2"]
                argv += [str(shadow / TEST_C), str(shadow / "firmware/src/dsp/breaks.c"),
                         "-lm", "-o", str(out_bin)]
                compiler_log = dest / (ident + "-compile.log")
                exitcode = command(argv, compiler_log)
                if exitcode:
                    raise RuntimeError(f"{ident} compile exit {exitcode}")
                pcm = dest / (ident + ".pcm")
                runlog = dest / (ident + "-run.log")
                env = os.environ.copy()
                if args.sanitizer:
                    env.update(ASAN_OPTIONS="detect_leaks=1:abort_on_error=1",
                               UBSAN_OPTIONS="halt_on_error=1")
                exitcode = command([str(out_bin), str(pcm)], runlog, env=env)
                if exitcode:
                    raise RuntimeError(f"{ident} execution exit {exitcode}")
                if pcm.stat().st_size != EXPECTED_PCM_BYTES:
                    raise RuntimeError(f"bad {ident} PCM length {pcm.stat().st_size}")
                lines = runlog.read_text().splitlines()
                cases = [line for line in lines if line.startswith("n=")]
                if len(cases) != EXPECTED_CASES or len(set(cases)) != EXPECTED_CASES:
                    raise RuntimeError(f"wrong {ident} case count or duplicate rows")
                if any(not re.search(r"audible=([1-9][0-9]*) silent=1$", line) for line in cases):
                    raise RuntimeError(f"inaudible or nonsilent {ident} case")
                event_lines = [line for line in lines if line.startswith("events:")]
                if len(event_lines) != 1 or not all(key in event_lines[0] for key in
                    ("pressed=1173", "released=1067", "bank_replace=364",
                     "reverse=270", "tempo=420")):
                    raise RuntimeError(f"nonreproducible {ident} event coverage")
                invalid_log = dest / (ident + "-missing-arg.log")
                if command([str(out_bin)], invalid_log) != 2:
                    raise RuntimeError(f"{ident} accepted missing PCM argument")
                pcm_pair[name] = pcm
                all_hashes.append(sha256(pcm))
                report["checks"].append({"name": ident,
                                          "compile_exit": 0, "execute_exit": 0,
                                          "missing_argument_exit": 2,
                                          "pcm_sha256": sha256(pcm),
                                          "compile_log_sha256": sha256(compiler_log),
                                          "run_log_sha256": sha256(runlog)})
            assert_equal(pcm_pair["legacy"], pcm_pair["fast"])
            for f in pcm_pair.values():
                f.unlink()  # digest evidence, no synthetic PCM upload
        if len(set(all_hashes)) != 1:
            raise RuntimeError("cross-compiler PCM mismatch")
        report["pcm_sha256"] = all_hashes[0]
        report["outcome"] = "PASS producer scoped PCM/STOP only; original sanitizer still FAIL"
        (dest / "manifest.json").write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(report["outcome"])
    print("SHA256:", report["pcm_sha256"])


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError, subprocess.TimeoutExpired) as exc:
        print("FAIL:", exc, file=sys.stderr)
        sys.exit(1)
