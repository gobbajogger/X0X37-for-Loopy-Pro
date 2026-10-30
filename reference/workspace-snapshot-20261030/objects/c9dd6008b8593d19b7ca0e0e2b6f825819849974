#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exact-source BREAK high-rate fractional-tempo continuity host regression.

The original, already-source-pinned fast-advance harness is copied into a temporary
location and changed *only* to exercise fractional playback increments, long
non-power-of-two samples and one live clock-tempo transition. Both variants compile
from the same pinned production breaks.c via BRK_FAST_REV_SKIP=0|1. No firmware
build, hardware-XRUN assertion, MIDI router, device or recorded samples.
"""
import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

EXPECTED = {
    "firmware/src/dsp/breaks.c": "9e1e607e2ddc3028383bfb6ebc85fbc7ac6864c5",
    "firmware/src/dsp/breaks.h": "6cb71d336c2c90fdc60a9594c2356f3a4379c264",
    "tests/host/breaks_reverse_fast_advance_test.c": "5e77b152253422d36ab7218669187d02318d21cb",
    "tests/host/run_breaks_reverse_fast_advance.py": "eec018339f607283dddc651c74b8d8eb5bfdd948",
    "firmware/src/dsp/ima_adpcm.h": "9ba16e4fe9f0cb643c95a68d540042ab2581d4de",
    "firmware/src/dsp/fastmath.h": "7d2e41bf70590bafa8af0a7376bb912bf763b098",
    "firmware/src/dsp/x0x_param.h": "f7c9bf22b0606946905790dc20497a2e4b2cb4a6",
    "firmware/src/seq/pattern.h": "9a0703418aae95b14f1b70905cfac916da38d7dd",
}
SIZES_OLD = "const uint32_t sizes[]={64,65,255,4097,176400};"
SIZES_NEW = "const uint32_t sizes[]={65,257,4097,176400,2116800};"
STEP_OLD = "float sp16=((float)n/32.0f)/(float)speed;"
STEP_NEW = "float sp16=((float)n/32.0f)/((float)speed + (trial ? 0.875f : 0.125f));"
TEMPO_BEFORE = "        uint64_t prior=x0x_host_ima_decode_calls;"
TEMPO_AFTER = ("        /* Mimic a live clock tempo change while two fading voices may exist. */\n"
               "        if(trial == 1 && j == BLOCKS / 2)\n"
               "            breaks_step(b,0,0,sp16*1.125f,1);\n"
               + TEMPO_BEFORE)
SUMMARY_OLD = "for n in (64, 65, 255, 4097, 176400):"
SUMMARY_NEW = "for n in (65, 257, 4097, 176400, 2116800):"


def git_blob(data):
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


def replace_exact(data, prior, after, path):
    if data.count(prior) != 1:
        raise ValueError(f"exact harness edit no longer supported in {path}")
    return data.replace(prior, after)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source-root", type=Path, required=True)
    p.add_argument("--output-dir", type=Path, required=True)
    p.add_argument("--compiler", choices=["gcc", "clang", "both"], default="both")
    p.add_argument("--sanitizer", action="store_true")
    args = p.parse_args()
    root = args.source_root.resolve()
    source_ids = {}
    for rel, expected in EXPECTED.items():
        actual = git_blob((root / rel).read_bytes())
        if actual != expected:
            raise ValueError(f"wrong Git source {rel}: {actual} != {expected}")
        source_ids[rel] = actual
    if args.output_dir.exists():
        p.error("output directory already exists")
    with tempfile.TemporaryDirectory(prefix="x0x-break-fraction-", dir="/tmp") as td:
        scratch = Path(td)
        old = (root / "tests/host/breaks_reverse_fast_advance_test.c").read_text()
        changed = replace_exact(old, SIZES_OLD, SIZES_NEW, "sizes")
        changed = replace_exact(changed, STEP_OLD, STEP_NEW, "fractional rate")
        changed = replace_exact(changed, TEMPO_BEFORE, TEMPO_AFTER, "live tempo change")
        (scratch / "breaks_reverse_fast_advance_test.c").write_text(changed)
        runner = (root / "tests/host/run_breaks_reverse_fast_advance.py").read_text()
        runner = replace_exact(runner, SUMMARY_OLD, SUMMARY_NEW, "result categories")
        fn = scratch / "fractional_inner.py"
        fn.write_text(runner)
        cmd = [sys.executable, "-B", str(fn), "--source-root", str(root),
               "--output-dir", str(args.output_dir), "--compiler", args.compiler]
        if args.sanitizer:
            cmd.append("--sanitizer")
        actual = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if not args.output_dir.exists():
            raise RuntimeError(f"inner regression failed before output: {actual.stderr}")
        (args.output_dir / "fractional-runner-stdout.txt").write_text(actual.stdout)
        (args.output_dir / "fractional-runner-stderr.txt").write_text(actual.stderr)
        if actual.returncode:
            raise RuntimeError(f"fractional native regression returned {actual.returncode}: {actual.stderr[-1800:]}")
        observed = json.loads((args.output_dir / "result.json").read_text())
        for comp in (["gcc", "clang"] if args.compiler == "both" else [args.compiler]):
            if len(observed[comp]) != 5 * 2 * 7:
                raise RuntimeError(f"wrong scenario matrix for {comp}")
            for row in observed[comp]:
                if row["n"] == 2116800 and row["direction"] == "R" and row["rate"] >= 32:
                    if row["fast-path_decode_calls"] > row["baseline_decode_calls"]:
                        raise RuntimeError(f"decoder work increased for long fast reverse: {row}")
        stamp = {
            "stage": "HOST_FRACTIONAL_TEMPO_SOURCE_EXACT_NOT_TARGET_BUILD_OR_QA",
            "compiled_source_head": "6db88ba43fe0f142f192711142af7726981940d0",
            "git_blobs": source_ids,
            "method": "fractional 1/8 and 7/8 playback-rate offsets; 7/8 trial changes clock tempo 12.5% mid-render; long, non-power-of-two, crossfade/NoteOff and STOP",
            "run_command": cmd,
            "run_exit": actual.returncode,
            "sanitizer": args.sanitizer,
            "compiler": args.compiler,
            "matched_cases_per_compiler": 140,
            "log_sha256": hashlib.sha256(actual.stdout.encode()).hexdigest(),
            "pcm_sha256": {cc: observed["raw_pcm_trace"][cc]["fast-path"]["sha256"]
                            for cc in (["gcc", "clang"] if args.compiler == "both" else [args.compiler])},
            "results": {cc: [x for x in observed[cc] if x["n"] == 2116800 and x["direction"] == "R"]
                        for cc in (["gcc", "clang"] if args.compiler == "both" else [args.compiler])},
        }
        (args.output_dir / "fractional-manifest.json").write_text(json.dumps(stamp, indent=2, sort_keys=True) + "\n")
        print(actual.stdout.strip())
        print("PASS fractional inc + mid-render tempo change, original-vs-fast PCM bit-equal")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError, KeyError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        sys.exit(1)
