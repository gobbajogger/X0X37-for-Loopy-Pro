#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""AU compare harness modes and render timing (factory presets vs matrix diagnostic)."""
from __future__ import annotations

import copy
import os
from dataclasses import dataclass
from typing import Any, Literal

CompareMode = Literal["preset", "matrix"]

QUICK_ENV_KEYS = (
    "AU_COMPARE_QUICK",
    "MIAMI_COMPARE_QUICK",
    "CHICAGO_AU_COMPARE_QUICK",
)
FULL_ENV_KEYS = ("AU_COMPARE_FULL",)
MODE_ENV = "AU_COMPARE_MODE"
BPM_ENV = "AU_COMPARE_BPM"
DEFAULT_COMPARE_BPM = 170.0

MIAMI_MATRIX_CONFIGS = (
    "carrier_filter",
    "carrier_open",
    "square_open",
    "sub_open",
    "mod_triangle",
    "mod_pulse",
    "mix_depth",
)


@dataclass(frozen=True)
class CompareTiming:
    frames: int
    note_on_frame: int
    note_off_frame: int
    steady_start: int
    steady_end: int
    label: str


FULL_TIMING = CompareTiming(
    frames=48000,
    note_on_frame=4800,
    note_off_frame=24000,
    steady_start=4800,
    steady_end=19200,
    label="full",
)

# Quick must keep the steady window entirely while the note is gated — same
# invariant as FULL (steady inside note_on..note_off). The old quick cut
# note_off at 12000 while steady ran to 19200, so env/corr/spectrum scored
# release+silence vs AU held notes and collapsed F_agg.
QUICK_TIMING = CompareTiming(
    frames=19200,
    note_on_frame=2400,
    note_off_frame=18000,
    steady_start=4800,
    steady_end=14400,
    label="quick",
)


def _truthy(val: str) -> bool:
    return val.strip().lower() in ("1", "true", "yes", "on")


def compare_mode() -> CompareMode:
    raw = os.environ.get(MODE_ENV, "preset").strip().lower()
    if raw in ("matrix", "diagnostic", "isolated"):
        return "matrix"
    return "preset"


def compare_full_length() -> bool:
    for key in FULL_ENV_KEYS:
        if _truthy(os.environ.get(key, "")):
            return True
    return False


def compare_quick_enabled() -> bool:
    if compare_full_length():
        return False
    for key in QUICK_ENV_KEYS:
        if _truthy(os.environ.get(key, "")):
            return True
    return False


def active_timing() -> CompareTiming:
    return QUICK_TIMING if compare_quick_enabled() else FULL_TIMING


def compare_bpm() -> float:
    raw = os.environ.get(BPM_ENV, "").strip()
    if raw:
        return float(raw)
    return DEFAULT_COMPARE_BPM


def apply_timing_to_scenario(scen: dict[str, Any]) -> dict[str, Any]:
    """Return a copy of a Chicago/Miami preset manifest with active render length."""
    out = copy.deepcopy(scen)
    t = active_timing()
    out["frames"] = t.frames
    out["noteOnFrame"] = t.note_on_frame
    out["noteOffFrame"] = t.note_off_frame
    out["steadyWindowStart"] = t.steady_start
    out["steadyWindowEnd"] = t.steady_end
    out["compareTiming"] = t.label
    out["bpm"] = compare_bpm()
    return out


def miami_matrix_frame_count() -> int:
    return active_timing().frames


def miami_matrix_note_off_frame() -> int:
    return active_timing().note_off_frame


def steady_window_from_manifest(scen: dict[str, Any]) -> tuple[int, int]:
    start = int(scen.get("steadyWindowStart", FULL_TIMING.steady_start))
    end = int(scen.get("steadyWindowEnd", FULL_TIMING.steady_end))
    return start, end


def harness_summary_line() -> str:
    mode = compare_mode()
    timing = active_timing().label
    if mode == "preset":
        role = "primary sign-off (factory preset patches, all presets)"
    else:
        role = "secondary diagnostic (Miami per-control matrix configs only)"
    return f"mode={mode} timing={timing} ({role})"
