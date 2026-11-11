#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Producer-scoped chunk-partition invariance check of pinned BREAK C inputs.

Original PR #93 has an independently owned fastmath negative-shift UBSan defect.
--sanitizer uses a verified, temporary PR #28 header projection, never a firmware edit.
Runs actual breaks.c, preserving output PCM across 1, 64, 128 and 256 frame calls.
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
TEST = "tests/host/breaks_chunk_partition_property_test.c"
TEST_BLOB = "88ae253ca2d00c8e55cfb8426654591817082b15"
PEER_BLOB = "a5c26c95cad0d91a548a8db073f59777e36d4e86"
PCM_SIZE = 10 * 32 * 256 * 4


def git_blob(data):
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cmd(argv, log, env=None):
    p = subprocess.run(argv, capture_output=True, text=True, timeout=180,
                       env=env, check=False)
    log.write_text("$ " + " ".join(map(str, argv)) + "\nexit=" + str(p.returncode) +
                   "\nSTDOUT:\n" + p.stdout + "\nSTDERR:\n" + p.stderr)
    return p.returncode


def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--compiler", choices=("gcc", "clang", "both"), default="both")
    parser.add_argument("--sanitizer", action="store_true")
    parser.add_argument("--peer-fastmath-file", type=Path)
    args = parser.parse_args()
    if args.output_dir.exists():
        parser.error("output directory already exists; evidence overwrite refused")
    if args.sanitizer != (args.peer_fastmath_file is not None):
        parser.error("sanitizer requires the independent PR28 header projection")
    root = args.source_root.resolve()
    pins = {}
    for path, expected in {**SOURCE_BLOBS, TEST: TEST_BLOB}.items():
        actual = git_blob((root / path).read_bytes())
        if actual != expected:
            raise RuntimeError(f"wrong source Git blob {path}: {actual} != {expected}")
        pins[path] = actual
    peer = None
    if args.peer_fastmath_file is not None:
        peer = args.peer_fastmath_file.read_bytes()
        if git_blob(peer) != PEER_BLOB:
            raise RuntimeError("wrong PR28 peer header Git blob")
    dest = args.output_dir.resolve()
    dest.mkdir(parents=True, exist_ok=False)
    findings = {"role": "Performance Engineering producer, not independent QA",
                "consumed_pr": 93,
                "consumed_firmware_subtree": "ee863f9cba6cbfb4e471d199ae408c7694acbac5",
                "source_git_blobs": pins,
                "sanitizer_peer_projection": PEER_BLOB if peer else None,
                "original_source_ubsan": "KNOWN_FAIL fastmath.h:57; independent PR28 owns repair",
                "target_build": "NOT_RUN", "device": "NOT_RUN",
                "scope": "real BREAK C render chunk invariance, 10 scenarios, five chunkings (1 to 256), live NoteOff, reverse, pitch, tempo, two-bank replacement, STOP; signed zeros canonicalized only",
                "checks": []}
    with tempfile.TemporaryDirectory(prefix="break-partitions-") as temp:
        shadow = Path(temp)
        for path in SOURCE_BLOBS:
            (shadow / path).parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(root / path, shadow / path)
        (shadow / TEST).parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(root / TEST, shadow / TEST)
        if peer is not None:
            (shadow / "firmware/src/dsp/fastmath.h").write_bytes(peer)
        digests = []
        compilers = ("gcc", "clang") if args.compiler == "both" else (args.compiler,)
        for cc in compilers:
            for fast in (0, 1):
                label = cc + "-" + ("fast" if fast else "legacy")
                binary = shadow / label
                argv = [cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
                        "-ffp-contract=off", "-I" + str(shadow),
                        "-DBRK_FAST_REV_SKIP=" + str(fast)]
                if args.sanitizer:
                    argv += ["-O1", "-g", "-fno-omit-frame-pointer",
                             "-fsanitize=address,undefined"]
                else:
                    argv += ["-O2"]
                argv += [str(shadow / TEST), str(shadow / "firmware/src/dsp/breaks.c"),
                         "-lm", "-o", str(binary)]
                compile_log = dest / (label + "-compile.log")
                if cmd(argv, compile_log):
                    raise RuntimeError(label + " compiler failed")
                output = dest / (label + ".pcm")
                run_log = dest / (label + "-run.log")
                env = os.environ.copy()
                if args.sanitizer:
                    env.update(ASAN_OPTIONS="detect_leaks=1:abort_on_error=1",
                               UBSAN_OPTIONS="halt_on_error=1")
                if cmd([str(binary), str(output)], run_log, env):
                    raise RuntimeError(label + " execution failed")
                if output.stat().st_size != PCM_SIZE:
                    raise RuntimeError(label + " unexpected PCM length")
                scenario_lines = [x for x in run_log.read_text().splitlines()
                                  if x.startswith("n=")]
                if len(scenario_lines) != 10 or len(set(scenario_lines)) != 10:
                    raise RuntimeError(label + " missing scenario evidence")
                if any(not re.fullmatch(r"n=\d+ seed=[01] partitions=5 exact=1 stopped=1", x)
                       for x in scenario_lines):
                    raise RuntimeError(label + " failed invariant")
                negative_log = dest / (label + "-missing-arg.log")
                if cmd([str(binary)], negative_log) != 2:
                    raise RuntimeError(label + " invalid-input negative not rejected")
                digest = sha256(output)
                digests.append(digest)
                findings["checks"].append({"label": label, "compile_exit": 0,
                                           "run_exit": 0, "invalid_arg_exit": 2,
                                           "pcm_sha256": digest,
                                           "compile_log_sha256": sha256(compile_log),
                                           "run_log_sha256": sha256(run_log)})
                output.unlink()  # no synthetic PCM is published
        if len(set(digests)) != 1:
            raise RuntimeError("PCM differs across compilers or reverse modes")
        findings["pcm_sha256"] = digests[0]
        findings["disposition"] = ("PASS: producer host PCM parity after zero-sign canonicalization; "
                                   "original-source sanitizer FAIL; target and QA NOT RUN")
        (dest / "manifest.json").write_text(json.dumps(findings, sort_keys=True,
                                                       indent=2) + "\n")
    print(findings["disposition"])
    print(findings["pcm_sha256"])

if __name__ == "__main__":
    try:
        run()
    except (OSError, RuntimeError, subprocess.TimeoutExpired, ValueError) as error:
        print("FAIL:", error, file=sys.stderr)
        sys.exit(1)
