#!/usr/bin/env python3
"""Reduce host observer JSONL without treating synthetic or partial data as a PASS.

Only complete DMA intervals wholly inside the requested window are tabulated.
The source's eng_prof counters are partial attribution, not total DSP work.
"""
from __future__ import annotations

import argparse
from contextlib import ExitStack
import csv
from fractions import Fraction
import hashlib
import json
import math
from pathlib import Path
import re

FIRMWARE_SOURCE = "94197fc839b00aff47b98ca02f3c6396e4a6eabf"
RUNNER_SOURCE = "31157d25caf7284080cdb5d86ab7e8e438120fd9"
PROFILE_NAMES = ["909", "808_and_shared_wait", "303A", "303B", "BREAK", "FX", "MASTER", "SEQ"]
WATCH_BYTES = {
    "x0x_dbg": 52, "audio_xruns": 4, "eng_prof": 32, "eng_prof_frames": 4,
    "eng_job_cost": 20, "eng_plan": 1, "eng_c1_mask": 1, "eng_stage": 4,
    "eng_mastered": 4, "eng_do_master": 1, "eng_job_arg": 4,
    "brk_silent_now": 4, "usr_nz": 3, "nloops": 4, "pack_n": 4,
    "kbuf": 1024, "dry_l": 1024, "dry_r": 1024,
}
STAGE_NAMES = {"eng_prof", "eng_prof_frames", "eng_job_cost", "eng_plan", "eng_c1_mask",
               "eng_stage", "eng_mastered", "eng_do_master", "eng_job_arg", "brk_silent_now"}
CSV_FIELDS = [
    "dma_seq", "scheduled_cycle", "deadline_cycle", "entry_cycle", "completion_cycle",
    "completion_uncertainty_cycles", "dma_period_us", "entry_latency_us", "irq_elapsed_us",
    "slack_lower_us", "slack_upper_us", "deadline_miss", "deadline_boundary_uncertain",
    "dma_edges_during_irq", "last_us", "last_us_writes", "late_delta", "xruns_delta",
    "nested_delta", "timer_irqs_delta", "ui_frames_delta", "halves_delta", "eng_prof_frames_delta",
    "eng_prof_delta", "eng_job_cost", "eng_plan", "eng_c1_mask", "eng_mastered", "eng_do_master",
    "break_loaded", "break_mix_nonzero_writes", "break_mix_peak_delta", "break_mix_invalid",
    "scene_before", "scene_after", "coherent",
]


def _uint(value, label, bits=64):
    if type(value) is not int or not 0 <= value < 1 << bits:
        raise ValueError(f"{label} must be an unsigned {bits}-bit integer")
    return value


def _delta(before, after):
    return (after - before) & 0xFFFFFFFF


def _snapshot(value, fields):
    if not isinstance(value, dict):
        raise ValueError("snapshot is not an object")
    if set(value) != set(fields):
        raise ValueError("snapshot fields do not match configured small watches")
    for name, (size, step) in fields.items():
        items = value.get(name)
        if not isinstance(items, list) or len(items) != size // step:
            raise ValueError(f"snapshot {name} has the wrong length")
        for item in items:
            _uint(item, f"snapshot {name}", step * 8)
    return value


def _snapshot_bytes(value, fields):
    result = {}
    for name, (_, step) in fields.items():
        result[name] = bytearray().join(item.to_bytes(step, "little") for item in value[name])
    return result


def _reject_constant(value):
    raise ValueError(f"nonfinite JSON number {value}")


class _Reducer:
    def __init__(self, start_ms, end_ms, slot, csv_writer, stage_file):
        self.start_ms, self.end_ms = Fraction(str(start_ms)), Fraction(str(end_ms))
        if self.start_ms < 0 or self.end_ms <= self.start_ms or slot not in (1, 2, 3):
            raise ValueError("require 0 <= start-ms < end-ms and slot 1, 2 or 3")
        self.slot, self.csv_writer, self.stage_file = slot, csv_writer, stage_file
        self.header = self.edge = self.active = self.finish = None
        self.start = self.end = None
        self.error_count, self.errors = 0, []
        self.last_event_cycle = 0
        self.first_dma_cycle = self.last_dma_deadline = None
        self.last_dma_seq = self.total_halves = self.total_resets = 0
        self.last_half_seq = 0
        self.window_dma = self.window_halves = self.coherent_halves = 0
        self.completed_misses = self.boundary_uncertain = self.max_edges_during_irq = 0
        self.late_total = self.xruns_total = self.loaded_halves = self.mix_halves = self.invalid_mix_halves = 0
        self.min_slack = self.max_last_us = None
        self.prof_total = [0] * 8
        self.snapshot_fields, self.scene_fields = {}, {}

    def fail(self, message):
        self.error_count += 1
        if len(self.errors) < 100:
            self.errors.append(message)

    def _contained(self, edge):
        return bool(edge is not None and edge["scheduled_cycle"] >= self.start and edge["deadline_cycle"] <= self.end)

    def _in_window(self, cycle):
        return self.start <= cycle <= self.end

    def _header(self, event):
        if self.header is not None:
            raise ValueError("duplicate observer header")
        if _uint(event.get("schema"), "schema", 8) != 1 or event.get("firmware_source") != FIRMWARE_SOURCE or event.get("runner_source") != RUNNER_SOURCE:
            raise ValueError("observer schema or exact source pin mismatch")
        hz = _uint(event.get("hz"), "hz")
        if hz != 5760000000 or event.get("cycle_ticks") != 16 or event.get("clock_unit") != "emulator_tick":
            raise ValueError("trace is not the pinned strict 360 MHz / 16-tick clock")
        if event.get("core1_time") != "global_slice_clock" or event.get("eng_prof_scope") != "guest_counters_with_known_core1_omissions":
            raise ValueError("missing attribution limitations in observer header")
        watches = event.get("watches")
        if not isinstance(watches, dict):
            raise ValueError("missing watch metadata")
        if not set(WATCH_BYTES).issubset(watches):
            raise ValueError("missing required observer watch")
        ranges, fields, scene_fields = [], {}, {}
        scene_bytes = 0
        for name, watch in watches.items():
            scene = name not in WATCH_BYTES
            if scene and re.fullmatch(r"scene_[a-z0-9_]+", name) is None:
                raise ValueError(f"unknown observer watch {name!r}")
            if not isinstance(watch, dict):
                raise ValueError(f"watch {name} metadata is not an object")
            address = _uint(watch.get("address"), f"watch {name} address", 32)
            size = _uint(watch.get("bytes"), f"watch {name} bytes", 32)
            step = _uint(watch.get("element_bytes"), f"watch {name} element size", 8)
            if scene:
                if not 1 <= size <= 512 or step != 1:
                    raise ValueError("scene watches require 1..512 raw bytes with element_bytes=1")
                scene_bytes += size
                scene_fields[name] = {"address": address, "bytes": size, "element_bytes": 1}
                if len(scene_fields) > 64 or scene_bytes > 4096:
                    raise ValueError("scene watch count or total bytes exceed 64 / 4096 limits")
            elif size != WATCH_BYTES[name] or step != (4 if size % 4 == 0 else 1):
                raise ValueError(f"watch {name} size does not match pinned firmware")
            if not 0x01C00000 <= address <= 0x01D00000 - size or (not scene and size % 4 == 0 and address % 4):
                raise ValueError(f"watch {name} is outside RAM or misaligned")
            if any(address < high and low < address + size for low, high in ranges):
                raise ValueError("overlapping watch metadata")
            ranges.append((address, address + size))
            if size < 1024:
                fields[name] = (size, step)
        self.header = event
        self.snapshot_fields, self.scene_fields = fields, scene_fields
        self.start, self.end = self.start_ms * hz / 1000, self.end_ms * hz / 1000

    def event(self, event):
        if not isinstance(event, dict):
            raise ValueError("trace record is not an object")
        kind = event.get("event")
        if kind == "observer":
            self._header(event)
            return
        if self.header is None:
            raise ValueError("event precedes observer header")
        if self.finish is not None:
            raise ValueError("event follows finish marker")
        cycle_key = {"dma": "observed_cycle", "half": "completion_cycle"}.get(kind, "cycle")
        cycle = _uint(event.get(cycle_key), cycle_key)
        if cycle < self.last_event_cycle:
            raise ValueError("event timestamps moved backwards")
        self.last_event_cycle = cycle
        if kind == "dma":
            self._dma(event)
        elif kind == "irq_enter":
            self._enter(event)
        elif kind == "store":
            self._store(event)
        elif kind == "half":
            self._half(event)
        elif kind == "reset":
            self.total_resets += 1
            if self._in_window(cycle):
                self.fail("chip reset during measurement window")
            if self.active and self._contained(self.active["edge"]):
                self.fail("reset interrupted a measured half")
            self.active = None
        elif kind == "incomplete":
            if self._in_window(cycle) or (self.active and self._contained(self.active["edge"])):
                self.fail("observer incomplete event: " + str(event.get("reason", "unspecified")))
        elif kind == "finish":
            for name in ("completed", "dma_edges", "resets"):
                _uint(event.get(name), f"finish {name}")
            if event.get("pending_seq") is not None:
                _uint(event["pending_seq"], "finish pending sequence")
            self.finish = event
        else:
            raise ValueError(f"unknown observer event {kind!r}")

    def _dma(self, event):
        for name in ("seq", "scheduled_cycle", "deadline_cycle", "address", "words", "half"):
            _uint(event.get(name), name)
        if event["seq"] != self.last_dma_seq + 1:
            self.fail("duplicate, missing or nonmonotonic DMA sequence")
        if event["words"] != 1024 or event["half"] not in (0, 1):
            self.fail("DMA event is not a 512-frame stereo half")
        if event["deadline_cycle"] <= event["scheduled_cycle"] or event["observed_cycle"] < event["scheduled_cycle"]:
            raise ValueError("invalid DMA deadline/order")
        if self.edge and event["scheduled_cycle"] != self.edge["deadline_cycle"]:
            # A reset/reconfiguration outside the requested window may restart DMA.
            if self._in_window(event["scheduled_cycle"]):
                self.fail("DMA interval discontinuity inside measurement window")
        self.edge = event
        self.last_dma_seq = event["seq"]
        if self.first_dma_cycle is None:
            self.first_dma_cycle = event["scheduled_cycle"]
        self.last_dma_deadline = event["deadline_cycle"]
        if self._contained(event):
            self.window_dma += 1

    def _enter(self, event):
        seq = _uint(event.get("seq"), "irq sequence")
        depth = _uint(event.get("depth"), "irq depth")
        before = _snapshot(event.get("snapshot"), self.snapshot_fields)
        if depth == 0 or self.edge is None or seq != self.edge["seq"]:
            raise ValueError("ALNK entry has no matching current DMA edge")
        if self.active is not None:
            self.fail("ALNK reentered before prior IRQ completion")
        if seq <= self.last_half_seq:
            self.fail("DMA half serviced more than once")
        self.active = {"entry": event, "edge": self.edge, "before": before,
                       "replay": _snapshot_bytes(before, self.snapshot_fields),
                       "us_count": 0, "us_value": None, "frame_count": 0, "frame_values": []}

    def _store(self, event):
        if self.active is None or event.get("seq") != self.active["entry"]["seq"]:
            raise ValueError("store has no matching active ALNK IRQ")
        name = event.get("name")
        if name not in self.snapshot_fields:
            raise ValueError("store is not a small configured watch")
        offset = _uint(event.get("offset"), "store offset", 32)
        width = _uint(event.get("width"), "store width", 32)
        core = _uint(event.get("core"), "core", 8)
        if width not in (1, 2, 4) or offset + width > self.snapshot_fields[name][0] or core not in (0, 1):
            raise ValueError("invalid watch store range or core")
        value = _uint(event.get("value"), "store value", width * 8)
        self.active["replay"][name][offset:offset + width] = value.to_bytes(width, "little")
        if name == "x0x_dbg" and offset == 32 and width == 4:
            self.active["us_count"] += 1
            self.active["us_value"] = value
            if core != 0:
                self.fail("last_us written outside core 0")
        if name == "eng_prof_frames" and offset == 0 and width == 4:
            self.active["frame_count"] += 1
            if len(self.active["frame_values"]) < 3:
                self.active["frame_values"].append(value)
            if core != 0:
                self.fail("eng_prof_frames written outside core 0")
        if (self.stage_file is not None and (name in STAGE_NAMES or name in self.scene_fields)
                and self._contained(self.active["edge"])):
            record = dict(event)
            record["ticks_since_irq_entry"] = event["cycle"] - self.active["entry"]["cycle"]
            if name in self.scene_fields:
                record["observation_kind"] = "scene_bytes"
            if name == "eng_job_arg" and width == 4 and offset == 0:
                record.update(stretch_frames=(value & 255) or 256, render_offset=value >> 20)
            self.stage_file.write(json.dumps(record, separators=(",", ":")) + "\n")

    def _half(self, event):
        self.total_halves += 1
        if self.active is None or event.get("seq") != self.active["entry"]["seq"]:
            raise ValueError("completed half has no matching ALNK entry")
        active, self.active = self.active, None
        seq, edge, before = event["seq"], active["edge"], active["before"]
        after = _snapshot(event.get("snapshot"), self.snapshot_fields)
        self.last_half_seq = seq
        if not self._contained(edge):
            return
        self.window_halves += 1
        errors_before = self.error_count
        replay_mismatch = [name for name, value in _snapshot_bytes(after, self.snapshot_fields).items()
                           if active["replay"][name] != value]
        if replay_mismatch:
            self.fail(f"half {seq}: store replay disagrees with final snapshot: {', '.join(replay_mismatch)}")
        end = _uint(event.get("completion_cycle"), "completion cycle")
        uncertainty = _uint(event.get("completion_uncertainty_cycles"), "completion uncertainty", 8)
        last_us = _uint(event.get("last_us"), "last_us", 32)
        if uncertainty > 1 or event.get("entry_cycle") != active["entry"]["cycle"] or end < event["entry_cycle"]:
            self.fail(f"half {seq}: invalid completion boundary")
        if event.get("dma_seq_at_completion") != self.last_dma_seq:
            self.fail(f"half {seq}: DMA completion sequence disagrees with edge stream")
        db, da = before["x0x_dbg"], after["x0x_dbg"]
        frames = _delta(before["eng_prof_frames"][0], after["eng_prof_frames"][0])
        frame_start = before["eng_prof_frames"][0]
        expected_frames = [(frame_start + 256) & 0xFFFFFFFF, (frame_start + 512) & 0xFFFFFFFF]
        if frames != 512 or active["frame_count"] != 2 or active["frame_values"] != expected_frames:
            self.fail(f"half {seq}: missing two coherent 256-frame render observations")
        if (event.get("last_us_writes") != 1 or active["us_count"] != 1 or
                active["us_value"] != last_us or da[8] != last_us):
            self.fail(f"half {seq}: missing or inconsistent last_us store")
        if (db[0] != 0x44424731 or da[0] != 0x44424731 or db[4] or da[4] or
                _delta(db[1], da[1]) != 1 or db[10] != da[10]):
            self.fail(f"half {seq}: incoherent debug snapshot")
        hz = self.header["hz"]
        ticks_us = 1000000 / hz
        elapsed = end - event["entry_cycle"]
        if elapsed * 24000000 >= (1 << 32) * hz:
            self.fail(f"half {seq}: profile-counter multi-wrap cannot be excluded")
        slack = edge["deadline_cycle"] - end
        miss, boundary = slack + uncertainty < 0, slack <= 0 <= slack + uncertainty
        crossed = self.last_dma_seq - seq
        if crossed < 0:
            self.fail(f"half {seq}: negative DMA advance")
        late, xruns = _delta(db[5], da[5]), _delta(before["audio_xruns"][0], after["audio_xruns"][0])
        prof = [_delta(a, b) for a, b in zip(before["eng_prof"], after["eng_prof"])]
        loaded_shape = [0, 0, 0]; loaded_shape[self.slot - 1] = 1
        loaded = all(s["usr_nz"] == loaded_shape and s["nloops"] == [1] and s["pack_n"] == [0]
                     for s in (before, after))
        mix = event.get("break_mix")
        if not isinstance(mix, dict):
            raise ValueError("missing BREAK mix witness")
        writes = _uint(mix.get("nonzero_writes"), "BREAK nonzero writes")
        peak, invalid = mix.get("peak_delta"), mix.get("invalid")
        if type(peak) not in (int, float) or not math.isfinite(peak) or peak < 0 or type(invalid) is not bool:
            raise ValueError("invalid BREAK mix witness")
        contribution = loaded and writes > 0 and peak > 0 and not invalid
        coherent = self.error_count == errors_before
        self.coherent_halves += coherent
        self.completed_misses += miss
        self.boundary_uncertain += boundary
        self.max_edges_during_irq = max(self.max_edges_during_irq, crossed)
        self.late_total += late
        self.xruns_total += xruns
        self.loaded_halves += loaded
        self.mix_halves += contribution
        self.invalid_mix_halves += invalid
        self.min_slack = slack * ticks_us if self.min_slack is None else min(self.min_slack, slack * ticks_us)
        self.max_last_us = last_us if self.max_last_us is None else max(self.max_last_us, last_us)
        self.prof_total = [a + b for a, b in zip(self.prof_total, prof)]
        row = dict(dma_seq=seq, scheduled_cycle=edge["scheduled_cycle"], deadline_cycle=edge["deadline_cycle"],
                   entry_cycle=event["entry_cycle"], completion_cycle=end, completion_uncertainty_cycles=uncertainty,
                   dma_period_us=(edge["deadline_cycle"] - edge["scheduled_cycle"]) * ticks_us,
                   entry_latency_us=(event["entry_cycle"] - edge["scheduled_cycle"]) * ticks_us,
                   irq_elapsed_us=elapsed * ticks_us, slack_lower_us=slack * ticks_us,
                   slack_upper_us=(slack + uncertainty) * ticks_us, deadline_miss=int(miss),
                   deadline_boundary_uncertain=int(boundary), dma_edges_during_irq=crossed,
                   last_us=last_us, last_us_writes=active["us_count"], late_delta=late, xruns_delta=xruns,
                   nested_delta=_delta(db[3], da[3]), timer_irqs_delta=_delta(db[6], da[6]),
                   ui_frames_delta=_delta(db[7], da[7]), halves_delta=_delta(db[1], da[1]),
                   eng_prof_frames_delta=frames, eng_prof_delta=json.dumps(prof),
                   eng_job_cost=json.dumps(after["eng_job_cost"]), eng_plan=after["eng_plan"][0],
                   eng_c1_mask=after["eng_c1_mask"][0], eng_mastered=after["eng_mastered"][0],
                   eng_do_master=after["eng_do_master"][0], break_loaded=int(loaded),
                   break_mix_nonzero_writes=writes, break_mix_peak_delta=peak,
                   break_mix_invalid=int(invalid),
                   scene_before=json.dumps({name: before[name] for name in self.scene_fields}),
                   scene_after=json.dumps({name: after[name] for name in self.scene_fields}),
                   coherent=int(coherent))
        if self.csv_writer is not None:
            self.csv_writer.writerow(row)

    def summary(self, trace_sha256):
        if self.header is None:
            self.fail("no valid observer header")
        if self.finish is None:
            self.fail("missing finish marker; trace may be truncated")
        else:
            if self.finish.get("completed") != self.total_halves or self.finish.get("dma_edges") != self.last_dma_seq:
                self.fail("finish counts do not match the complete event stream")
            if self.finish.get("resets") != self.total_resets:
                self.fail("finish reset count disagrees with stream")
            pending = self.active["entry"]["seq"] if self.active else None
            if self.finish.get("pending_seq") != pending:
                self.fail("finish pending IRQ disagrees with stream")
            if self.end is not None and self.finish["cycle"] < self.end:
                self.fail("emulator stopped before requested window end")
        if self.active and self._contained(self.active["edge"]):
            self.fail("measured DMA half has no completed IRQ snapshot")
        if (self.start is None or self.first_dma_cycle is None or self.first_dma_cycle > self.start or
                self.last_dma_deadline < self.end):
            self.fail("DMA observations do not span requested window")
        if not self.window_dma or not self.window_halves:
            self.fail("no complete measured DMA/render halves")
        if self.window_halves > self.window_dma:
            self.fail("more measured completions than DMA edges")
        status = "INCOMPLETE" if self.error_count else "OBSERVED"
        loaded = self.window_halves > 0 and self.loaded_halves == self.window_halves
        return {
            "schema": 1, "status": status, "trace_sha256": trace_sha256,
            "firmware_source": FIRMWARE_SOURCE, "runner_source": RUNNER_SOURCE,
            "owner_scene_exact": False, "representative_scene_qualified": False,
            "clock": {"hz": self.header["hz"] if self.header else None, "unit": "emulator_tick", "cycle_ticks": 16},
            "window": {"start_ms": float(self.start_ms), "end_ms": float(self.end_ms),
                       "selection": "DMA interval wholly contained in window",
                       "dma_edges": self.window_dma, "completed_halves": self.window_halves,
                       "coherent_halves": self.coherent_halves},
            "deadline": {"completed_irq_deadline_misses": self.completed_misses,
                         "unserviced_dma_edges": max(0, self.window_dma - self.window_halves),
                         "boundary_uncertain_halves": self.boundary_uncertain,
                         "max_dma_edges_during_irq": self.max_edges_during_irq,
                         "minimum_slack_lower_us": self.min_slack,
                         "max_guest_last_us": self.max_last_us,
                         "firmware_late_delta": self.late_total, "firmware_xruns_delta": self.xruns_total},
            "attribution": {"eng_prof_scope": "guest_counters_with_known_core1_omissions",
                            "eng_prof_names": PROFILE_NAMES, "eng_prof_delta_sum": self.prof_total,
                            "eng_prof_unit": "24 MHz timer ticks for pinned perf_cyc_ok=0 firmware",
                            "eng_job_cost": "smoothed normalized ticks per 256 frames; not raw per-half work",
                            "core1_timestamps": "global slice clock; not independent core-1 instruction timings",
                            "shared_wait_bucket": "808", "total_dsp_cost": "NOT_DERIVABLE"},
            "break_evidence": {"slot": self.slot,
                               "loaded": "OBSERVED" if loaded and status == "OBSERVED" else "NOT_OBSERVED",
                               "mix_contribution": "OBSERVED" if loaded and self.mix_halves and not self.invalid_mix_halves and status == "OBSERVED" else "NOT_OBSERVED",
                               "contributing_halves": self.mix_halves, "invalid_mix_halves": self.invalid_mix_halves,
                               "audibility": "UNVERIFIED",
                               "limitation": "A finite current-stretch dry-bus delta is numerical contribution; final-output or human audibility requires separate evidence."},
            "unverified_scene_fields": ["Chicago engine and arp state", "Juno engine and held triad",
                                        "909 four-on-the-floor pattern", "808 inactivity", "final-output BREAK audibility",
                                        "owner installed package/model and musical settings"],
            "scene_observations": {"fields": self.scene_fields, "interpretation": "UNVERIFIED",
                                   "scope": "Raw configured bytes only; offsets and values require separate source/layout and ELF verification."},
            "trailing_pending_seq": self.active["entry"]["seq"] if self.active else None,
            "error_count": self.error_count, "errors": self.errors,
            "evidence_scope": "observer trace only; no hardware qualification or guessed PASS",
        }


def analyze_trace(trace_path, start_ms, end_ms, slot=1, out_dir=None):
    """Read JSONL once; bind closed output files by hash when a new out_dir is given."""
    with ExitStack() as stack:
        source = stack.enter_context(Path(trace_path).open("rb"))
        out = Path(out_dir) if out_dir is not None else None
        writer = stage_file = None
        if out is not None:
            out.mkdir(parents=True, exist_ok=False)
            csv_file = stack.enter_context((out / "per_half.csv").open("w", newline=""))
            writer = csv.DictWriter(csv_file, fieldnames=CSV_FIELDS)
            writer.writeheader()
            stage_file = stack.enter_context((out / "stage_events.jsonl").open("w"))
        reducer = _Reducer(start_ms, end_ms, slot, writer, stage_file)
        digest = hashlib.sha256()
        for line_number, raw in enumerate(source, 1):
            digest.update(raw)
            if not raw.strip():
                continue
            try:
                event = json.loads(raw, parse_constant=_reject_constant)
                reducer.event(event)
            except (ValueError, TypeError, KeyError, IndexError, UnicodeError) as error:
                reducer.fail(f"line {line_number}: {error}")
        result = reducer.summary(digest.hexdigest())
    # ExitStack has closed/flushed both artifacts before their bytes are hashed.
    # Stream the files: a long observer trace can have a large store timeline.
    if out is not None:
        for filename, key in (("per_half.csv", "per_half_csv_sha256"),
                              ("stage_events.jsonl", "stage_events_sha256")):
            digest = hashlib.sha256()
            with (out / filename).open("rb") as artifact:
                for chunk in iter(lambda: artifact.read(1 << 20), b""):
                    digest.update(chunk)
            result[key] = digest.hexdigest()
        (out / "summary.json").write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--start-ms", required=True)
    parser.add_argument("--end-ms", required=True)
    parser.add_argument("--slot", type=int, choices=(1, 2, 3), default=1)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    try:
        result = analyze_trace(args.trace, args.start_ms, args.end_ms, args.slot, args.out)
    except (OSError, ValueError) as error:
        parser.exit(2, f"trace: {error}\n")
    print(json.dumps(result, indent=2, allow_nan=False))
    return 0 if result["status"] == "OBSERVED" else 2


if __name__ == "__main__":
    raise SystemExit(main())
