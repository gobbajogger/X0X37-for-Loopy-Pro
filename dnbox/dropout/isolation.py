"""Associate BREAK-only input receipts with numerical host-observer evidence.

This is an independent-run witness, not a decomposition of the mixed owner
scene. Boundary snapshots alone cannot exclude hidden or earlier live notes.
A positive result is conditional on Runner's separate execution receipt proving
a fresh exact fixture, the exact derived key script, and no USB/note injections.
No guest execution, guest writes, device operations, or new profiling occurs.
"""
from __future__ import annotations

import csv
import hashlib
import json
import math
from pathlib import Path
import re

from scene import FIRMWARE_SOURCE, check_snapshot, check_window
from pcm import RUNNER_SOURCE, HZ

FULL_KEYS_SHA256 = "fc0b36dba5836217e989f110bd580e720106a71e8741ab2c1726252789212965"
BREAK_ONLY_KEYS_SHA256 = "8d9adb5687d5af1cfe37231e527e2786b79aa2d8cf9c2fd968535594f8ac853b"
_REMOVED_LINES = tuple(f"8000:k{key}@127,\n".encode() for key in (5, 12, 19, 26)) + tuple(
    f"8100:k{key}^,\n".encode() for key in (5, 12, 19, 26))
_ISOLATED_ZERO_FIELDS = {"scene_a_usb_keys": 128, "scene_b_usb_keys": 128, "scene_909_hit": 22}
_DIGEST = re.compile(r"[0-9a-f]{64}\Z")
EXECUTION_CONTRACT = {
    "status": "CONDITIONAL_ON_SEPARATE_EXECUTION_RECEIPT",
    "verified_by_this_function": False,
    "requirements": [
        "Runner records a fresh fixture made from the exact hash-verified package and the verified BREAK sample.",
        "Runner records the exact derived BREAK-only key script SHA256 and its full-scene parent SHA256.",
        "Runner records no USB script and no other note-injection routes or inherited runtime/project state.",
        "Runner binds this run's trace and PCM file hashes to that execution receipt.",
    ],
    "full_keys_sha256": FULL_KEYS_SHA256,
    "break_only_keys_sha256": BREAK_ONLY_KEYS_SHA256,
    "limitation": "Zero sampled USB ownership counters and drum hit rows do not prove arbitrary traces contain no hidden or earlier notes.",
}


def break_only_keys(full_keys_bytes):
    """Remove exactly the four909-step press/release pairs; retain other bytes."""
    if not isinstance(full_keys_bytes, bytes) or hashlib.sha256(full_keys_bytes).hexdigest() != FULL_KEYS_SHA256:
        raise ValueError("unknown full key recipe SHA256; BREAK-only derivation requires the pinned recipe")
    lines = full_keys_bytes.splitlines(keepends=True)
    if any(lines.count(line) != 1 for line in _REMOVED_LINES):
        raise ValueError("pinned key recipe does not contain the exact eight909 toggle event lines")
    derived = b"".join(line for line in lines if line not in _REMOVED_LINES)
    if hashlib.sha256(derived).hexdigest() != BREAK_ONLY_KEYS_SHA256:
        raise ValueError("derived BREAK-only key recipe SHA256 mismatch")
    return derived


def _digest(value):
    return isinstance(value, str) and _DIGEST.fullmatch(value) is not None


def _bounds(value, label):
    if not isinstance(value, dict):
        raise ValueError(f"{label}: missing observed window")
    start, end = value.get("start_ms"), value.get("end_ms")
    if (any(type(x) not in (int, float) or not math.isfinite(x) for x in (start, end))
            or start < 0 or end <= start):
        raise ValueError(f"{label}: invalid observed window bounds")
    return {"start_ms": start, "end_ms": end}


def _nonfinite(value):
    raise ValueError(f"nonfinite JSON value {value}")


def _nonnegative_integer(value, label):
    if not isinstance(value, str) or re.fullmatch(r"[0-9]+", value) is None:
        raise ValueError(f"{label}: expected nonnegative CSV integer")
    return int(value)


def check_break_only(csv_path, scene, trace_summary, pcm_summary):
    """Return a receipt-bound numerical witness, never human-audibility approval.

The explicit observed interval comes from the trace receipt and must equal the
PCM receipt's interval. It is retained alongside the scene's declared interval;
a shorter diagnostic interval is labeled partial. CSV rows use fully contained
DMA intervals, while PCM uses scheduled DMA events in[start,end). Those policies
are not silently equated or relabeled as output from the same numbered IRQ.
"""
    report = {
        "schema": 1, "status": "INCOMPLETE", "owner_scene_exact": False,
        "representative_scene_qualified": False,
        "input_isolation": EXECUTION_CONTRACT,
        "human_audibility": "UNVERIFIED", "full_scene_break_dac_mix": "NOT_PROVEN",
        "808_live_inactivity": "UNVERIFIED",
        "evidence_scope": "Separate BREAK-only-input numerical witness, conditional on the recorded fresh-fixture/derived-keys/no-USB execution contract.",
        "window_policy": "Same millisecond bounds; CSV contains full DMA intervals and PCM selects scheduled DMA reads before their IRQ renders.",
        "incomplete_reasons": [], "failures": [],
    }
    errors = report["incomplete_reasons"]
    if not isinstance(trace_summary, dict) or not isinstance(pcm_summary, dict):
        errors.append("trace and PCM summaries must both be objects")
        return report
    try:
        observed_window = _bounds(trace_summary.get("window"), "trace")
        pcm_window = _bounds(pcm_summary.get("window"), "PCM")
        declared_window = _bounds(scene, "declared scene")
        report.update(declared_window=declared_window, observed_window=observed_window,
                      partial_duration=observed_window != declared_window)
        if pcm_window != observed_window:
            errors.append("PCM and trace observed windows differ")
        integrity = check_window(csv_path, scene, trace_summary,
                                 observed_start_ms=observed_window["start_ms"],
                                 observed_end_ms=observed_window["end_ms"])
    except (OSError, ValueError, TypeError, KeyError, UnicodeError, csv.Error) as error:
        errors.append(str(error))
        return report
    report["csv_sha256"] = integrity["csv_sha256"]
    report["trace_complete"] = integrity["trace_complete"]
    if not integrity["trace_complete"]:
        errors.extend(integrity["incomplete_reasons"])
    # The full-scene check is expected to reject empty synth ownership/drum rows.
    # Its trace/layout/digest checks are reused; every boundary is independently
    # checked below, without relying on its bounded failure-example list.
    trace_hash = trace_summary.get("trace_sha256")
    if not _digest(trace_hash) or trace_hash != pcm_summary.get("trace_sha256"):
        errors.append("PCM summary belongs to a different or unidentified trace SHA256")
    report["trace_sha256"] = trace_hash
    if (trace_summary.get("status") != "OBSERVED"
            or trace_summary.get("firmware_source") != FIRMWARE_SOURCE
            or trace_summary.get("runner_source") != RUNNER_SOURCE):
        errors.append("trace summary is not observed at the pinned source profile")
    source_profile = pcm_summary.get("source_profile", {})
    if (not isinstance(source_profile, dict)
            or source_profile.get("firmware_source") != FIRMWARE_SOURCE
            or source_profile.get("runner_source") != RUNNER_SOURCE):
        errors.append("PCM source profile differs from the pinned trace")
    for label, summary in (("trace", trace_summary), ("PCM", pcm_summary)):
        clock = summary.get("clock", {})
        if not isinstance(clock, dict) or (clock.get("hz"), clock.get("unit"), clock.get("cycle_ticks")) != (HZ, "emulator_tick", 16):
            errors.append(f"{label} summary has the wrong clock")
    for name in ("pcm_sha256", "window_pcm_sha256"):
        value = pcm_summary.get(name)
        report[name] = value
        if not _digest(value):
            errors.append(f"PCM summary is missing a valid {name}")

    rows = boundaries = failed_boundaries = loaded_rows = contributing_rows = invalid_rows = 0
    digest = hashlib.sha256()
    try:
        with Path(csv_path).open("rb") as source:
            def hashed_lines():
                for raw in source:
                    digest.update(raw)
                    yield raw.decode("utf-8")
            reader = csv.DictReader(hashed_lines())
            needed = {"dma_seq", "coherent", "scene_before", "scene_after", "break_loaded",
                      "break_mix_nonzero_writes", "break_mix_peak_delta", "break_mix_invalid"}
            if not needed.issubset(reader.fieldnames or []):
                raise ValueError("per-half CSV is missing BREAK isolation evidence columns")
            for row in reader:
                rows += 1
                sequence = _nonnegative_integer(row["dma_seq"], "dma_seq")
                if row["coherent"] != "1":
                    raise ValueError(f"DMA{sequence}: incoherent CSV half")
                for side in ("before", "after"):
                    snapshot = json.loads(row["scene_" + side], parse_constant=_nonfinite)
                    result = check_snapshot(snapshot, scene)
                    violations = [v for v in result["violations"] if v["field"] not in _ISOLATED_ZERO_FIELDS]
                    values = result.get("observed", {})
                    for name, count in _ISOLATED_ZERO_FIELDS.items():
                        if values.get(name) != [0] * count:
                            violations.append({"field": name, "reason": "BREAK-only input witness requires every sampled value to be zero"})
                    boundaries += 1
                    if violations:
                        failed_boundaries += 1
                        if len(report["failures"]) < 20:
                            report["failures"].append({"dma_seq": sequence, "boundary": side, "violations": violations})
                if row["break_loaded"] not in ("0", "1") or row["break_mix_invalid"] not in ("0", "1"):
                    raise ValueError(f"DMA{sequence}: malformed BREAK loaded/invalid flags")
                loaded_rows += row["break_loaded"] == "1"
                invalid_rows += row["break_mix_invalid"] == "1"
                writes = _nonnegative_integer(row["break_mix_nonzero_writes"], "BREAK writes")
                peak = float(row["break_mix_peak_delta"])
                if not math.isfinite(peak) or peak < 0:
                    raise ValueError(f"DMA{sequence}: invalid BREAK mix delta")
                contributing_rows += row["break_loaded"] == "1" and row["break_mix_invalid"] == "0" and writes > 0 and peak > 0
    except (OSError, ValueError, TypeError, KeyError, UnicodeError, csv.Error) as error:
        errors.append(str(error))
    if digest.hexdigest() != integrity["csv_sha256"]:
        errors.append("CSV bytes changed between integrity and isolation checks")
    if rows != integrity["rows"] or not rows or boundaries != rows * 2:
        errors.append("isolation did not inspect both boundaries of every CSV half")
    break_evidence = trace_summary.get("break_evidence", {})
    if not isinstance(break_evidence, dict):
        break_evidence = {}
    break_ok = (loaded_rows == rows and rows > 0 and contributing_rows > 0 and invalid_rows == 0
                and break_evidence.get("loaded") == "OBSERVED"
                and break_evidence.get("mix_contribution") == "OBSERVED"
                and break_evidence.get("contributing_halves") == contributing_rows
                and break_evidence.get("invalid_mix_halves") == 0)
    samples = pcm_summary.get("samples", {})
    if not isinstance(samples, dict):
        samples = {}
    count, nonzero, peak = (samples.get(name) for name in ("scalar_samples", "nonzero_samples", "peak_absolute_int16"))
    pcm_ok = (pcm_summary.get("status") == "OBSERVED" and pcm_summary.get("numeric_output") == "NONZERO"
              and all(type(value) is int for value in (count, nonzero, peak))
              and 0 < nonzero <= count and 0 < peak <= 32768)
    report.update(rows=rows, checked_boundaries=boundaries, failing_boundaries=failed_boundaries,
                  loaded_rows=loaded_rows, contributing_rows=contributing_rows, invalid_mix_rows=invalid_rows,
                  break_numeric_contribution=break_ok, pcm_numeric_output_nonzero=pcm_ok,
                  pcm_samples=samples, boundary_state_matches=bool(rows and not failed_boundaries))
    if not errors:
        report["status"] = ("NUMERICAL_BREAK_OUTPUT_OBSERVED" if not failed_boundaries and break_ok and pcm_ok
                            else "NOT_OBSERVED")
    return report
