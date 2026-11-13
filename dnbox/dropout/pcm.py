#!/usr/bin/env python3
"""Measure numeric DAC capture values mapped to actual observer DMA events.

Source contract: fm1-soc/src/lib.rs advances peripherals, emits each DMA event,
then appends that event's 1024 RAM words as clamped signed int16 PCM samples.
fm1-live/src/main.rs writes the entire periph.audio vector as little-endian
samples for FM1_DNBOX_PCM. This path has no synthetic leading boot silence.

The DMA read precedes that edge's IRQ render. A selected chunk is therefore NOT
identified as the output rendered by the same numbered IRQ. This reducer checks
the DMA-to-file mapping only. It does not qualify IRQ timing, the musical scene,
BREAK attribution, or human audibility. No WAV sample rate is guessed.
"""
from __future__ import annotations

import argparse
from array import array
from fractions import Fraction
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

FIRMWARE_SOURCE = "94197fc839b00aff47b98ca02f3c6396e4a6eabf"
RUNNER_SOURCE = "31157d25caf7284080cdb5d86ab7e8e438120fd9"
HZ = 5_760_000_000
CYCLE_TICKS = 16
T4_HZ, FRAME_T4 = 24_000_000, 544
WORDS_PER_DMA = 1024
FRAMES_PER_DMA = WORDS_PER_DMA // 2
BYTES_PER_DMA = WORDS_PER_DMA * 2
PERIOD_TICKS = FRAMES_PER_DMA * FRAME_T4 * HZ // T4_HZ


def _uint(value, label, bits=64):
    if type(value) is not int or not 0 <= value < 1 << bits:
        raise ValueError(f"{label}: expected unsigned {bits}-bit integer")
    return value


def _nonfinite(value):
    raise ValueError(f"nonfinite JSON value {value}")


def _window(start_ms, end_ms):
    if type(start_ms) is bool or type(end_ms) is bool:
        raise ValueError("window bounds must be finite milliseconds")
    try:
        start, end = Fraction(str(start_ms)), Fraction(str(end_ms))
    except (ValueError, ZeroDivisionError) as error:
        raise ValueError("window bounds must be finite milliseconds") from error
    if not 0 <= start < end:
        raise ValueError("require 0 <= start_ms < end_ms")
    return start, end, start * HZ / 1000, end * HZ / 1000


def _dma_index(trace_path, start_tick, end_tick):
    """Validate the full trace's DMA/file ordering, without judging IRQ records."""
    digest = hashlib.sha256()
    header = finish = None
    count = selected_count = 0
    first_dma = last_dma = first_selected = last_selected = None
    previous_cycle = None
    allowed = {"dma", "irq_enter", "store", "half", "incomplete", "reset", "finish"}
    with Path(trace_path).open("rb") as source:
        for line_number, raw in enumerate(source, 1):
            digest.update(raw)
            if not raw.strip():
                continue
            try:
                event = json.loads(raw, parse_constant=_nonfinite)
                if not isinstance(event, dict):
                    raise ValueError("trace record is not an object")
                kind = event.get("event")
                if kind == "observer":
                    if header is not None or count or finish is not None:
                        raise ValueError("duplicate/misplaced observer header")
                    if (event.get("schema") != 1 or type(event.get("schema")) is not int
                            or event.get("firmware_source") != FIRMWARE_SOURCE
                            or event.get("runner_source") != RUNNER_SOURCE
                            or event.get("hz") != HZ or type(event.get("hz")) is not int
                            or event.get("clock_unit") != "emulator_tick"
                            or event.get("cycle_ticks") != CYCLE_TICKS):
                        raise ValueError("observer source/schema/strict clock pin mismatch")
                    header = event
                    continue
                if header is None:
                    raise ValueError("event precedes pinned observer header")
                if finish is not None:
                    raise ValueError("event follows finish marker")
                if kind not in allowed:
                    raise ValueError(f"unsupported trace event {kind!r}")
                key = {"dma": "observed_cycle", "half": "completion_cycle"}.get(kind, "cycle")
                cycle = _uint(event.get(key), key)
                if previous_cycle is not None and cycle < previous_cycle:
                    raise ValueError("trace event timestamps moved backwards")
                previous_cycle = cycle
                if kind == "reset":
                    raise ValueError("reset prevents the pinned uninterrupted DMA/PCM mapping")
                if kind == "finish":
                    if _uint(event.get("resets"), "finish resets") != 0:
                        raise ValueError("finish records a reset")
                    if _uint(event.get("dma_edges"), "finish dma_edges") != count:
                        raise ValueError("finish DMA count disagrees with complete event stream")
                    finish = event
                    continue
                if kind != "dma":
                    # These events are not evidence of PCM-source attribution.
                    # The independent trace reducer owns render/IRQ validation.
                    continue
                sequence = _uint(event.get("seq"), "DMA seq")
                if sequence != count + 1:
                    raise ValueError("DMA sequence must be consecutive from1; event missing or duplicated")
                words = _uint(event.get("words"), "DMA words", 32)
                if words != WORDS_PER_DMA:
                    raise ValueError("DMA chunk must contain1024 scalar stereo samples")
                scheduled = _uint(event.get("scheduled_cycle"), "DMA scheduled_cycle")
                deadline = _uint(event.get("deadline_cycle"), "DMA deadline_cycle")
                half = _uint(event.get("half"), "DMA half", 32)
                if half != (sequence - 1) % 2:
                    raise ValueError("DMA halves must alternate0/1 starting at0")
                if deadline - scheduled != PERIOD_TICKS or cycle < scheduled:
                    raise ValueError("DMA period/order differs from exact emulator DAC clock")
                if last_dma is not None and scheduled != last_dma["deadline_cycle"]:
                    raise ValueError("DMA scheduled periods are discontinuous")
                count += 1
                if first_dma is None:
                    first_dma = event
                last_dma = event
                if start_tick <= scheduled < end_tick:
                    selected_count += 1
                    if first_selected is None:
                        first_selected = event
                    last_selected = event
            except (ValueError, TypeError, KeyError, UnicodeError) as error:
                raise ValueError(f"trace line{line_number}: {error}") from error
    if header is None or finish is None:
        raise ValueError("trace lacks pinned header or terminal finish marker")
    if finish["cycle"] < end_tick:
        raise ValueError("trace ends before the requested window end")
    return {"trace_sha256": digest.hexdigest(), "dma_count": count,
            "selected_count": selected_count, "first_dma": first_dma, "last_dma": last_dma,
            "first_selected": first_selected, "last_selected": last_selected,
            "finish_cycle": finish["cycle"]}


def _stats(count, nonzero, peak, squares):
    if count == 0:
        rms = dbfs = None
        dbfs_kind = "not_observed"
    else:
        rms = math.sqrt(squares / count)
        dbfs = 20 * math.log10(rms / 32768) if squares else None
        dbfs_kind = "finite" if squares else "negative_infinity"
    return {"scalar_samples": count, "nonzero_samples": nonzero,
            "peak_absolute_int16": peak if count else None,
            "sum_squares_int16": squares, "rms_int16": rms,
            "rms_full_scale": rms / 32768 if rms is not None else None,
            "rms_dbfs": dbfs, "rms_dbfs_kind": dbfs_kind,
            "dbfs_reference": "signed int16 full-scale amplitude32768"}


def analyze_pcm(pcm_path, trace_path, start_ms, end_ms):
    """Return numeric DAC-buffer evidence or raise ValueError for invalid mapping.

The selected chunks are those whose DMA scheduled timestamp lies in [start,end).
All1024 scalar samples read at each selected DMA event are included; sample file
offsets start with DMA seq1, not firmware boot time. Window chunks are not
partially trimmed or relabeled as same-IRQ rendered output.
"""
    start, end, start_tick, end_tick = _window(start_ms, end_ms)
    indexed = _dma_index(trace_path, start_tick, end_tick)
    pcm_path = Path(pcm_path)
    expected_bytes = indexed["dma_count"] * BYTES_PER_DMA
    actual_bytes = pcm_path.stat().st_size
    if actual_bytes != expected_bytes:
        raise ValueError(f"PCM byte count{actual_bytes} differs from DMA-derived count{expected_bytes}; truncated, padded or unmatched capture")
    first, last = indexed["first_selected"], indexed["last_selected"]
    first_seq, last_seq = (first["seq"], last["seq"]) if first is not None else (None, None)
    full_digest, window_digest = hashlib.sha256(), hashlib.sha256()
    # Per-channel count/nonzero/peak/squares; integer accumulation is exact.
    channels = [[0, 0, 0, 0], [0, 0, 0, 0]]
    with pcm_path.open("rb") as source:
        for sequence in range(1, indexed["dma_count"] + 1):
            chunk = source.read(BYTES_PER_DMA)
            if len(chunk) != BYTES_PER_DMA:
                raise ValueError("PCM changed or truncated while being read")
            full_digest.update(chunk)
            if first_seq is None or not first_seq <= sequence <= last_seq:
                continue
            window_digest.update(chunk)
            samples = array("h")
            samples.frombytes(chunk)
            if samples.itemsize != 2:
                raise ValueError("host signed-short width cannot decode int16 PCM")
            if sys.byteorder != "little":
                samples.byteswap()
            for i, value in enumerate(samples):
                stat = channels[i & 1]
                stat[0] += 1
                stat[1] += value != 0
                stat[2] = max(stat[2], abs(value))
                stat[3] += value * value
        if source.read(1):
            raise ValueError("PCM grew while being read")
    combined = [channels[0][0] + channels[1][0], channels[0][1] + channels[1][1],
                max(channels[0][2], channels[1][2]), channels[0][3] + channels[1][3]]
    if combined[0] != indexed["selected_count"] * WORDS_PER_DMA:
        raise ValueError("selected PCM size disagrees with selected DMA events")
    byte_first = (first_seq - 1) * BYTES_PER_DMA if first_seq is not None else None
    byte_end = last_seq * BYTES_PER_DMA if last_seq is not None else None
    return {
        "schema": 1, "status": "OBSERVED" if combined[0] else "NO_SAMPLES",
        "numeric_output": "NONZERO" if combined[1] else "DIGITAL_ZERO" if combined[0] else "NOT_OBSERVED",
        "pcm_sha256": full_digest.hexdigest(), "window_pcm_sha256": window_digest.hexdigest(),
        "trace_sha256": indexed["trace_sha256"],
        "pcm_bytes": actual_bytes, "dma_count": indexed["dma_count"],
        "format": "signed16 little-endian interleaved L/R; two scalar samples per stereo frame",
        "clock": {"hz": HZ, "unit": "emulator_tick", "cycle_ticks": CYCLE_TICKS,
                  "dma_period_ticks": PERIOD_TICKS,
                  "sample_rate_stereo_frames_per_second": {"numerator": T4_HZ, "denominator": FRAME_T4}},
        "source_profile": {"firmware_source": FIRMWARE_SOURCE, "runner_source": RUNNER_SOURCE,
                           "artifact_attestation": "header source pins only; actual input hashes are checked by Runner"},
        "window": {"start_ms": float(start), "end_ms": float(end),
                   "selection": "DMA scheduled timestamp in [start_ms,end_ms); include every scalar sample in each selected chunk",
                   "selected_dma_count": indexed["selected_count"], "first_dma_seq": first_seq, "last_dma_seq": last_seq,
                   "first_scheduled_tick": first["scheduled_cycle"] if first else None,
                   "last_scheduled_tick": last["scheduled_cycle"] if last else None,
                   "first_observed_tick": first["observed_cycle"] if first else None,
                   "last_observed_tick": last["observed_cycle"] if last else None,
                   "pcm_byte_first": byte_first, "pcm_byte_end_exclusive": byte_end,
                   "scalar_sample_first": byte_first // 2 if byte_first is not None else None,
                   "scalar_sample_end_exclusive": byte_end // 2 if byte_end is not None else None,
                   "stereo_frames": combined[0] // 2},
        "samples": _stats(*combined),
        "channels": {"left": _stats(*channels[0]), "right": _stats(*channels[1])},
        "evidence_scope": "Numeric DAC capture buffer values mapped to DMA read events; no musical-source or human-audibility attribution.",
        "same_irq_render_attribution": "NOT_CLAIMED; DMA reads precede the corresponding IRQ render",
        "boot_padding": "none; PCM begins with DMA seq1 regardless of boot delay",
        "trace_irq_qualification": "NOT_EVALUATED", "scene_qualification": "NOT_EVALUATED",
        "break_attribution": "NOT_EVALUATED", "human_audibility": "UNVERIFIED",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pcm", type=Path, required=True)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--start-ms", required=True)
    parser.add_argument("--end-ms", required=True)
    args = parser.parse_args()
    try:
        result = analyze_pcm(args.pcm, args.trace, args.start_ms, args.end_ms)
    except (OSError, ValueError, struct.error) as error:
        parser.exit(2, f"pcm: {error}\n")
    print(json.dumps(result, indent=2, allow_nan=False))
    return 0 if result["status"] == "OBSERVED" else 2


if __name__ == "__main__":
    raise SystemExit(main())
