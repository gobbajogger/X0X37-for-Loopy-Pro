"""Check fresh 808 activity predicates in the pinned observer's selected stretches.

This checks the existing JB_808 predicate and mix guard, not zero 808 CPU work.
Stage events have no header; the reducer summary supplies checked source/watch
metadata and a digest of these exact bytes. Original append order is retained,
including core1 stores sharing the global slice clock with core0.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re

FIRMWARE_SOURCE = "94197fc839b00aff47b98ca02f3c6396e4a6eabf"
RUNNER_SOURCE = "31157d25caf7284080cdb5d86ab7e8e438120fd9"
ELF_SHA256 = {
    "elite": "720f851c83a62fbef71fe52ad84e669409a69fde1d1fb313427d7942ed3ca1ea",
    "pro": "7e67c3b33fe2e690e5f913b5d048727a715c70daca06ddd326a10e37eacb9535",
}
SYMBOL = "d808_was_active"
SYMBOL_ADDRESS, SYMBOL_BYTES = 29393605, 1
WATCH = "scene_808_active"
WATCH_METADATA = {"address": SYMBOL_ADDRESS, "bytes": SYMBOL_BYTES, "element_bytes": 1}
STAGE_NAMES = {"eng_prof", "eng_prof_frames", "eng_job_cost", "eng_plan", "eng_c1_mask",
               "eng_stage", "eng_mastered", "eng_do_master", "eng_job_arg", "brk_silent_now"}
LIMITATIONS = [
    "The predicate is read at JB_808 entry within each stretch, not continuously between observations.",
    "A zero predicate excludes the guarded JB_808 render and mix_808 contribution for that stretch.",
    "The earlier eng_808_split decision can dispatch split-body prefix work before this predicate read; that work is not excluded.",
    "808 setup, pan settling and shared wait attribution may still consume CPU; zero CPU cost is not established.",
    "No activity state is inferred for unserviced DMA edges or an interval outside the reducer's window.",
    "This does not establish the other instruments, final-output audibility, hardware timing, or exact owner reproduction.",
]


def activity_map(symbols, elf_sha256):
    """Validate the actual retained ELF symbol before emitting its byte watch."""
    if elf_sha256 not in ELF_SHA256.values():
        raise ValueError("808 activity watch requires an exact retained ELF SHA256")
    if not isinstance(symbols, dict):
        raise ValueError("808 activity symbols must come from the verified ELF")
    value = symbols.get(SYMBOL)
    if isinstance(value, dict):
        value = (value.get("address"), value.get("size_bytes"))
    if (not isinstance(value, (tuple, list)) or len(value) != 2
            or any(type(item) is not int for item in value)
            or tuple(value) != (SYMBOL_ADDRESS, SYMBOL_BYTES)):
        raise ValueError("d808_was_active: missing or mismatched retained ELF address/size")
    return f"{WATCH} 0x{SYMBOL_ADDRESS:08x} {SYMBOL_BYTES}\n"


def _uint(value, label, bits=64):
    if type(value) is not int or not 0 <= value < 1 << bits:
        raise ValueError(f"{label} must be an unsigned {bits}-bit integer")
    return value


def _nonfinite(value):
    raise ValueError(f"nonfinite JSON number {value}")


def _store_event(event):
    if not isinstance(event, dict) or event.get("event") != "store":
        raise ValueError("selected stage artifact must contain store records only")
    seq = _uint(event.get("seq"), "DMA sequence")
    if not seq:
        raise ValueError("DMA sequence must be positive")
    _uint(event.get("cycle"), "global event cycle")
    core = _uint(event.get("core"), "core", 8)
    width = _uint(event.get("width"), "store width", 8)
    _uint(event.get("offset"), "store offset", 32)
    if core not in (0, 1) or width not in (1, 2, 4):
        raise ValueError("invalid core or store width")
    _uint(event.get("value"), "store value", width * 8)
    name = event.get("name")
    if not isinstance(name, str) or (name not in STAGE_NAMES and re.fullmatch(r"scene_[a-z0-9_]+", name) is None):
        raise ValueError("unknown stage artifact watch")
    if name == "eng_stage" and (event["offset"] != 0 or width != 4 or event["value"] > 4):
        raise ValueError("eng_stage requires a complete aligned word store with value0..4")
    if name == WATCH and (event["offset"] != 0 or width != 1 or event.get("observation_kind") != "scene_bytes"):
        raise ValueError("scene_808_active requires a fresh labeled byte store")
    return event


class _Half:
    def __init__(self, seq):
        self.row = {"dma_seq": seq, "stretch_resets": 0, "stage2_completions": 0,
                    "complete_stretches": 0, "activity_stores": 0, "zero_stores": 0,
                    "active_stores": 0, "intervals_with_one_fresh_store": 0,
                    "inactive_stretches": 0, "error_count": 0, "issues": []}
        self.open_stores = None

    def fail(self, reason):
        self.row["error_count"] += 1
        if len(self.row["issues"]) < 20:
            self.row["issues"].append(reason)

    def event(self, event):
        value = event["value"]
        if event["name"] == WATCH:
            self.row["activity_stores"] += 1
            self.row["active_stores" if value else "zero_stores"] += 1
            if self.open_stores is None:
                self.fail("activity store is outside an eng_stage0..2 interval")
            else:
                self.open_stores.append(value)
        elif event["name"] == "eng_stage":
            if value == 0:
                self.row["stretch_resets"] += 1
                if event["core"] != 0:
                    self.fail("stretch reset occurred outside core0")
                if self.open_stores is not None:
                    self.fail("new stretch reset precedes the previous stage2 completion")
                self.open_stores = []
            elif value == 2:
                self.row["stage2_completions"] += 1
                if self.open_stores is None:
                    self.fail("stage2 completion has no preceding stretch reset")
                else:
                    self.row["complete_stretches"] += 1
                    if len(self.open_stores) != 1:
                        self.fail("stretch must contain exactly one fresh 808 activity store")
                    else:
                        self.row["intervals_with_one_fresh_store"] += 1
                        self.row["inactive_stretches"] += self.open_stores[0] == 0
                    self.open_stores = None
            elif value == 1 and self.open_stores is None:
                self.fail("stage1 has no open stretch reset")
            elif value > 2 and self.open_stores is not None:
                self.fail("later mix stage precedes the stage2 completion")

    def finish(self):
        if self.open_stores is not None:
            self.fail("last stretch has no stage2 completion")
        if self.row["stretch_resets"] < 2 or self.row["stage2_completions"] < 2:
            self.fail("a selected 512-frame half needs at least two complete stretches")
        if self.row["stretch_resets"] != self.row["stage2_completions"]:
            self.fail("stretch reset and stage2 completion counts differ")
        self.row["coverage_complete"] = self.row["error_count"] == 0
        return self.row


def check_808_inactive(stage_events_path, trace_summary):
    """Check every selected half's fresh predicates, without sorting store events.

    INACTIVE_OBSERVED requires complete bound coverage. A bound nonzero store can
    establish ACTIVE_OBSERVED even when other coverage is incomplete. Unbound or
    absent evidence yields UNVERIFIED. None of these statuses qualifies a scene.
    """
    provenance, reasons, rows = [], [], []
    error_count = 0

    def fail(reason):
        nonlocal error_count
        error_count += 1
        if len(reasons) < 40:
            reasons.append(reason)

    report = trace_summary if isinstance(trace_summary, dict) else {}
    if (type(report.get("schema")) is not int or report.get("schema") != 1
            or report.get("firmware_source") != FIRMWARE_SOURCE or report.get("runner_source") != RUNNER_SOURCE):
        provenance.append("missing or mismatched pinned reducer source/schema")
    observations = report.get("scene_observations")
    fields = observations.get("fields") if isinstance(observations, dict) else None
    metadata = fields.get(WATCH) if isinstance(fields, dict) else None
    if (not isinstance(metadata, dict) or metadata != WATCH_METADATA
            or any(type(metadata.get(key)) is not int for key in WATCH_METADATA)):
        provenance.append("missing or mismatched exact scene_808_active byte-watch metadata")
    window = report.get("window")
    window = window if isinstance(window, dict) else {}
    expected, coherent, edges = (window.get(key) for key in ("completed_halves", "coherent_halves", "dma_edges"))
    valid_counts = (type(expected) is int and expected > 0 and type(coherent) is int and coherent == expected
                    and type(edges) is int and edges >= expected)
    if (report.get("status") != "OBSERVED" or type(report.get("error_count")) is not int
            or report.get("error_count") != 0 or not valid_counts):
        fail("trace summary does not establish complete coherent selected-half coverage")
    digest, current, previous_seq, previous_cycle = hashlib.sha256(), None, 0, None
    read_complete = True
    try:
        with Path(stage_events_path).open("rb") as source:
            for number, raw in enumerate(source, 1):
                digest.update(raw)
                if not raw.endswith(b"\n"):
                    fail(f"line{number}: unterminated store record; artifact may be truncated")
                try:
                    event = _store_event(json.loads(raw, parse_constant=_nonfinite))
                    if previous_cycle is not None and event["cycle"] < previous_cycle:
                        fail(f"line{number}: global event cycle moved backwards")
                    previous_cycle = event["cycle"]
                    if current is None or event["seq"] != current.row["dma_seq"]:
                        if current is not None:
                            rows.append(current.finish())
                        if event["seq"] <= previous_seq:
                            fail(f"line{number}: duplicate or nonmonotonic DMA half group")
                        previous_seq = event["seq"]
                        current = _Half(event["seq"])
                    current.event(event)
                except (ValueError, TypeError, KeyError, UnicodeError) as error:
                    fail(f"line{number}: {error}")
    except OSError as error:
        read_complete = False
        fail(f"cannot read the complete stage artifact: {error}")
    if current is not None:
        rows.append(current.finish())
    actual_digest = digest.hexdigest() if read_complete else None
    expected_digest = report.get("stage_events_sha256")
    if (not isinstance(expected_digest, str) or re.fullmatch(r"[0-9a-f]{64}", expected_digest) is None
            or actual_digest != expected_digest):
        provenance.append("stage_events_sha256 is absent or does not identify these exact artifact bytes")
    for row in rows:
        if not row["coverage_complete"]:
            fail(f"half{row['dma_seq']}: {'; '.join(row['issues'])}")
    if not rows or len(rows) != expected:
        fail("observed stage half count differs from the reducer's completed-half count")
    provenance_verified = not provenance
    coverage = provenance_verified and error_count == 0
    active_stores = sum(row["active_stores"] for row in rows)
    inactive = coverage and active_stores == 0
    active = provenance_verified and active_stores > 0
    return {
        "status": "ACTIVE_OBSERVED" if active else "INACTIVE_OBSERVED" if inactive else "UNVERIFIED",
        "all_observed_stretches_inactive": inactive,
        "mix_guard_excluded_at_all_observed_stretches": inactive,
        "predicate_active_observed": active,
        "provenance_verified": provenance_verified, "coverage_complete": coverage,
        "firmware_source": FIRMWARE_SOURCE, "runner_source": RUNNER_SOURCE,
        "stage_events_sha256": actual_digest, "expected_stage_events_sha256": expected_digest,
        "watch_metadata": dict(WATCH_METADATA), "observed_window": window,
        "selected_halves": len(rows), "expected_completed_halves": expected,
        "observed_dma_sequences": [row["dma_seq"] for row in rows],
        "total_stretches": sum(row["complete_stretches"] for row in rows),
        "activity_stores": sum(row["activity_stores"] for row in rows), "active_stores": active_stores,
        "per_half": rows, "unobserved_dma_edges": edges - expected if valid_counts else None,
        "inactivity_on_unobserved_dma_edges": "UNVERIFIED", "zero_808_cpu_cost": "UNVERIFIED",
        "owner_scene_exact": False, "representative_scene_qualified": False, "audibility": "UNVERIFIED",
        "evidence_scope": "Fresh d808_was_active predicates at all covered JB_808 entries, bounded by each eng_stage0..2 stretch interval; mix_808 is excluded when that predicate is zero.",
        "incomplete_reasons": provenance + reasons, "error_count": len(provenance) + error_count,
        "source_references": ["engine.c:2209", "engine.c:2257-2261", "engine.c:2383-2384",
                              "engine.c:2418-2422", "engine.c:2710", "engine.c:2718-2722"],
        "limitations": list(LIMITATIONS),
    }
