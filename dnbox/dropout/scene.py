"""Interpret read-only scene snapshots for the exact retained DnBOX images.

Offsets below were emitted by the supplied JieLi clang4 target, from exact
94197fc8 public headers, using pi32v2/r3/fprev1. Three aggregate sizes match
both exact ELF symbol tables. No host ABI, guessed offsets, firmware rebuild,
or DWARF claim is used. Values prove sampled boundary state only; they do not
prove unchanged state between samples, human audibility, or owner reproduction.
"""
from __future__ import annotations

import csv
import hashlib
import json
import math
from pathlib import Path
import struct

FIRMWARE_SOURCE = "94197fc839b00aff47b98ca02f3c6396e4a6eabf"
ELF_SHA256 = {
    "elite": "720f851c83a62fbef71fe52ad84e669409a69fde1d1fb313427d7942ed3ca1ea",
    "pro": "7e67c3b33fe2e690e5f913b5d048727a715c70daca06ddd326a10e37eacb9535",
}
LAYOUT_RECEIPT = {
    "source_commit": FIRMWARE_SOURCE,
    "proof_sha256": "126d0601f2d31f826f5feed571c21866e06e6423884e7b86927c01198f40899e",
    "compiler_sha256": "c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f",
    "toolchain_archive_sha256": "ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19",
    "constant_record_sha256": "25aa423b3abaa29c66e04941c83020c8543c1f797fcad123c4a9fcdf4233d44e",
    "abi": "ELF32 little endian, machine0xf1; pi32v2/r3/fprev1; pointers4, float4",
    "derivation": "target-compiler offsetof/sizeof; three aggregate ELF size cross-checks; no DWARF",
    "runtime_measurement": False,
}
RAM_BASE, RAM_BYTES = 0x01C00000, 1 << 20
PART_BYTES = 5128
# Exact retained symbols have the same addresses and sizes in Elite and Pro.
SYMBOLS = {"original_bass": (0x01C3C3BC, 10256), "seq": (0x01C0A7C0, 440),
           "proj": (0x01C0B8E4, 36420)}
# (raw watch name, containing ELF symbol, byte offset within that symbol, bytes)
# Original runtime kind/status follow the DSP/key state, at the END of each part.
WATCHES = (
    ("scene_a_kind", "original_bass", 5124, 1),
    ("scene_a_status", "original_bass", 5125, 1),
    ("scene_a_usb_keys", "original_bass", 3664, 128),
    ("scene_a_arp_enable", "original_bass", 318, 1),
    ("scene_b_kind", "original_bass", 10252, 1),
    ("scene_b_status", "original_bass", 10253, 1),
    ("scene_b_usb_keys", "original_bass", 8792, 128),
    ("scene_seq_pat", "seq", 0, 4),
    ("scene_seq_playing", "seq", 8, 1),
    ("scene_seq_ppat", "seq", 9, 5),
    ("scene_seq_ch", "seq", 34, 5),
    ("scene_seq_part_off", "seq", 30, 1),
    ("scene_seq_mute", "seq", 40, 4),
    ("scene_seq_vmute", "seq", 48, 4),
    ("scene_seq_bpm", "seq", 80, 4),
    ("scene_909_hit", "proj", 2752, 88),
    ("scene_909_len", "proj", 2848, 1),
    ("scene_909_rate", "proj", 2849, 1),
    ("scene_808_hit", "proj", 2852, 88),
    ("scene_808_len", "proj", 2948, 1),
    ("scene_808_rate", "proj", 2949, 1),
    ("scene_brk_steps", "proj", 3368, 4),
    ("scene_brk_set", "proj", 3372, 12),
    ("scene_brk_slot_a", "proj", 3384, 1),
    ("scene_brk_rate", "proj", 3387, 1),
    ("scene_brk_slice", "proj", 3388, 16),
)
PAT0_ADDRESS = SYMBOLS["proj"][0] + 2752
BRK_BCHANCE = 9  # Included once in scene_brk_set; no overlapping byte watch.
# Separate optional alias of the existing d808_was_active byte, checked in both
# retained ELF symbol tables. activity_map owns its symbol validation/map suffix.
ACTIVITY_METADATA = {"scene_808_active": {"address": 29393605, "bytes": 1, "element_bytes": 1}}
ACTIVITY_SCOPE = "Last sampled d808_was_active predicate only; per-stretch activity coverage is unverified."
SCOPE = "Required scene state at observed ALNK entry/completion boundaries only."
LIMITATIONS = [
    "Boundary snapshots do not prove unchanged scene state between observations.",
    "Held-key bytes are ownership counters, not note velocities or active voice counts.",
    "Empty 808 hit rows prove the sampled pattern is empty, not absence of residual live voices.",
    "An optional zero 808 activity predicate at a boundary does not prove inactivity in every stretch.",
    "Patch/factory identity, FX settings and most BREAK generator controls remain unverified.",
    "BREAK configuration is separate from loaded sample, numerical contribution and human audibility evidence.",
    "Owner settings and installed artifact remain unconfirmed; this scene is representative.",
]


def _symbols_checked(symbols, elf_sha256):
    if elf_sha256 not in ELF_SHA256.values():
        raise ValueError("scene layout requires one of the two exact retained ELF SHA256 values")
    if not isinstance(symbols, dict):
        raise ValueError("scene symbols must be a canonical-name mapping from the verified ELF")
    checked = {}
    for name, expected in SYMBOLS.items():
        value = symbols.get(name)
        if isinstance(value, dict):
            value = (value.get("address"), value.get("size_bytes"))
        if not isinstance(value, (tuple, list)) or len(value) != 2 or any(type(x) is not int for x in value):
            raise ValueError(f"{name}: missing address/size from the verified ELF")
        if tuple(value) != expected:
            raise ValueError(f"{name}: retained ELF address/size mismatch; cannot reuse this target layout")
        checked[name] = tuple(value)
    return checked


def scene_metadata(symbols, elf_sha256):
    """Validate the actual ELF aggregate symbols and return raw-byte watch metadata."""
    symbols = _symbols_checked(symbols, elf_sha256)
    result, ranges = {}, []
    for name, symbol, offset, size in WATCHES:
        base, extent = symbols[symbol]
        address = base + offset
        if offset < 0 or offset + size > extent or not 1 <= size <= 512:
            raise ValueError(f"{name}: layout field leaves its checked aggregate")
        if not RAM_BASE <= address <= RAM_BASE + RAM_BYTES - size:
            raise ValueError(f"{name}: layout field is outside target RAM")
        if any(address < high and low < address + size for low, high in ranges):
            raise ValueError("overlapping scene watches")
        ranges.append((address, address + size))
        result[name] = {"address": address, "bytes": size, "element_bytes": 1}
    return result


def scene_map(symbols, elf_sha256):
    """Map suffix; caller must obtain symbols and SHA256 from the actual ELF bytes."""
    metadata = scene_metadata(symbols, elf_sha256)
    return "".join(f"{name} 0x{value['address']:08x} {value['bytes']}\n"
                   for name, value in metadata.items())


def snapshot_from_ram(ram_bytes, symbols, elf_sha256):
    """Read a final RAM dump. This is not a time window or per-half observation."""
    if len(ram_bytes) != RAM_BYTES:
        raise ValueError("final RAM dump must contain exactly the emulator's 1 MiB RAM")
    metadata = scene_metadata(symbols, elf_sha256)
    return {name: list(ram_bytes[value["address"] - RAM_BASE:value["address"] - RAM_BASE + value["bytes"]])
            for name, value in metadata.items()}


def _expected(scene):
    if (not isinstance(scene, dict) or scene.get("firmware_source") != FIRMWARE_SOURCE
            or scene.get("owner_scene_exact") is not False or scene.get("kind") != "representative"):
        raise ValueError("scene checker requires the pinned representative, non-exact scene declaration")
    intended = scene.get("intended")
    if not isinstance(intended, dict):
        raise ValueError("scene is missing intended settings")
    bpm = intended.get("tempo_bpm")
    if type(bpm) not in (int, float) or not math.isfinite(bpm) or not 0 < bpm < 1000:
        raise ValueError("scene intended tempo_bpm must be a finite positive supported value")
    for label, engine, notes, channel in (("synth_a", "Chicago", [48, 52, 55], 2),
                                            ("synth_b", "Juno", [60, 64, 67], 3)):
        part = intended.get(label)
        if (not isinstance(part, dict) or part.get("engine") != engine
                or part.get("held_notes") != notes or part.get("usb_channel") != channel):
            raise ValueError(f"{label}: this checker is bound to the declared Chicago/Juno representative triads")
    if intended["synth_a"].get("internal_arp") != "on":
        raise ValueError("representative Chicago internal arp must be on")
    brk, drum, empty = intended.get("break"), intended.get("909"), intended.get("808")
    if (not isinstance(brk, dict)
            or (brk.get("slot"), brk.get("loop_index"), brk.get("steps"), brk.get("b_chance")) != (1, 0, "0xffff", 0)
            or any(type(brk.get(name)) is not int for name in ("slot", "loop_index", "b_chance"))):
        raise ValueError("representative BREAK requires physical slot1, logical loop0, steps0xffff and B chance0")
    if not isinstance(drum, dict) or (drum.get("voice"), drum.get("steps")) != ("BD", "0x1111"):
        raise ValueError("representative 909 requires BD four-on-the-floor0x1111")
    if not isinstance(empty, dict) or (empty.get("all_steps"), empty.get("external_notes")) != ("empty", "none"):
        raise ValueError("representative 808 must declare empty sequence and no external notes")
    # Compare the actual binary32 value that the target can store, not a tolerance.
    return struct.unpack("<f", struct.pack("<f", bpm))[0]


def _decode(snapshot):
    if not isinstance(snapshot, dict):
        raise ValueError("scene snapshot must be an object of raw byte arrays")
    expected = {name: size for name, _, _, size in WATCHES}
    if "scene_808_active" in snapshot:
        expected["scene_808_active"] = 1
    if (any(not isinstance(name, str) for name in snapshot)
            or {name for name in snapshot if name.startswith("scene_")} != set(expected)):
        raise ValueError("missing or unexpected scene snapshot fields")
    result = {}
    for name, size in expected.items():
        value = snapshot[name]
        if (not isinstance(value, list) or len(value) != size
                or any(type(x) is not int or not 0 <= x <= 255 for x in value)):
            raise ValueError(f"{name}: expected {size} raw bytes")
        if name in ("scene_909_hit", "scene_808_hit"):
            result[name] = list(struct.unpack("<22I", bytes(value)))
        elif name in ("scene_seq_pat", "scene_seq_mute", "scene_seq_vmute", "scene_brk_steps"):
            result[name] = int.from_bytes(bytes(value), "little")
        elif name == "scene_seq_bpm":
            number = struct.unpack("<f", bytes(value))[0]
            if not math.isfinite(number):
                raise ValueError("scene_seq_bpm: nonfinite target binary32")
            result[name] = number
        else:
            result[name] = value[0] if size == 1 else value
    return result


def check_snapshot(snapshot, scene):
    """Check one observed boundary/final snapshot; no claim of continuous state."""
    bpm = _expected(scene)
    report = {"status": "REJECTED", "required_state_matches": False,
              "owner_scene_exact": False, "evidence_scope": SCOPE,
              "layout_receipt": LAYOUT_RECEIPT, "audibility": "UNVERIFIED",
              "808_live_inactivity": "UNVERIFIED",
              "sampled_808_activity": {"status": "NOT_OBSERVED", "value": None, "scope": ACTIVITY_SCOPE},
              "limitations": LIMITATIONS, "violations": []}
    try:
        values = _decode(snapshot)
    except ValueError as error:
        report["violations"].append({"field": "snapshot", "reason": str(error)})
        return report
    report["observed"] = values
    expected = {
        "scene_a_kind": 0, "scene_a_status": 1, "scene_a_arp_enable": 1,
        "scene_b_kind": 3, "scene_b_status": 1,
        "scene_seq_pat": PAT0_ADDRESS, "scene_seq_playing": 1,
        "scene_seq_ppat": [0] * 5, "scene_seq_ch": [9, 10, 1, 2, 3],
        "scene_seq_part_off": 0, "scene_seq_mute": 0, "scene_seq_vmute": 0,
        "scene_seq_bpm": bpm, "scene_909_hit": [0x1111] + [0] * 21,
        "scene_909_len": 16, "scene_909_rate": 0,
        "scene_808_hit": [0] * 22, "scene_808_len": 16, "scene_808_rate": 0,
        # engine.c:3038-3077,3154-3159: this is the packed zero-based loop-list
        # index. The fixture's physical FSMP slot1 is independently checked by
        # usr_nz/nloops/pack_n in the trace reducer; it is NOT pattern.slot_a1.
        "scene_brk_steps": 0xFFFF, "scene_brk_slot_a": scene["intended"]["break"]["loop_index"], "scene_brk_rate": 0,
    }
    if "scene_808_active" in values:
        expected["scene_808_active"] = 0
        report["sampled_808_activity"].update(status="OBSERVED", value=values["scene_808_active"])
    for name, notes in (("scene_a_usb_keys", [48, 52, 55]), ("scene_b_usb_keys", [60, 64, 67])):
        expected[name] = [int(key in notes) for key in range(128)]
    for name, wanted in expected.items():
        if values[name] != wanted:
            report["violations"].append({"field": name, "expected": wanted, "observed": values[name]})
    chance = values["scene_brk_set"][BRK_BCHANCE]
    if chance != 0:
        report["violations"].append({"field": "scene_brk_set[BRK_BCHANCE]", "expected": 0, "observed": chance})
    report["required_state_matches"] = not report["violations"]
    if report["required_state_matches"]:
        report["status"] = "OBSERVED_MATCH"
    return report


def _no_nonfinite(value):
    raise ValueError(f"nonfinite JSON number {value}")


def _window(start, end, label):
    if (any(type(value) not in (int, float) or not math.isfinite(value) for value in (start, end))
            or start < 0 or end <= start):
        raise ValueError(f"{label}: window needs finite increasing nonnegative millisecond bounds")
    return {"start_ms": start, "end_ms": end}


def check_window(csv_path, scene, trace_summary=None, *, observed_start_ms=None, observed_end_ms=None):
    """Check both sampled boundaries of every reduced CSV half.

The reducer must also have validated trace completeness and exact watch metadata,
and its CSV digest must identify these exact parsed bytes. By default its window
must equal the scene declaration. A paired explicit override permits a diagnostic
interval while retaining both windows in the receipt. A boundary match never
qualifies the full scene or evaluates final-output/human audibility. Unserviced
DMA edges remain explicit; no boundary state is invented for them.
"""
    _expected(scene)
    declared_window = _window(scene.get("start_ms"), scene.get("end_ms"), "declared scene")
    override = observed_start_ms is not None or observed_end_ms is not None
    if (observed_start_ms is None) != (observed_end_ms is None):
        raise ValueError("observed window override requires both start and end")
    wanted_window = (_window(observed_start_ms, observed_end_ms, "observed override")
                     if override else declared_window)
    path = Path(csv_path)
    failures, incomplete = [], []
    rows = coherent_rows = matching_rows = boundaries = failing_boundaries = 0
    activity_presence = set()
    previous = None
    csv_digest = hashlib.sha256()
    with path.open("rb") as source:
        def hashed_lines():
            for raw in source:
                csv_digest.update(raw)
                yield raw.decode("utf-8")
        reader = csv.DictReader(hashed_lines())
        if not {"dma_seq", "coherent", "scene_before", "scene_after"}.issubset(reader.fieldnames or []):
            raise ValueError("per-half CSV is missing coherent scene boundary columns")
        for row in reader:
            rows += 1
            try:
                seq = int(row["dma_seq"])
                if seq < 1 or (previous is not None and seq <= previous):
                    raise ValueError("duplicate/nonmonotonic DMA sequence in scene CSV")
                previous = seq
                if row["coherent"] not in ("0", "1"):
                    raise ValueError("CSV coherent must be0 or1")
                coherent_rows += row["coherent"] == "1"
                row_matches = row["coherent"] == "1"
                for side in ("before", "after"):
                    snapshot = json.loads(row["scene_" + side], parse_constant=_no_nonfinite)
                    result = check_snapshot(snapshot, scene)
                    if "observed" in result:
                        activity_presence.add("scene_808_active" in result["observed"])
                    boundaries += 1
                    row_matches &= result["required_state_matches"]
                    if not result["required_state_matches"]:
                        failing_boundaries += 1
                        if len(failures) < 20:
                            failures.append({"dma_seq": seq, "boundary": side, "violations": result["violations"]})
                matching_rows += row_matches
            except (ValueError, TypeError, KeyError) as error:
                if len(incomplete) < 20:
                    incomplete.append(f"CSV row{rows}: {error}")
    if not rows:
        incomplete.append("no observed half rows")
    if rows != coherent_rows:
        incomplete.append("not all CSV halves are coherent")
    expected_metadata = scene_metadata(SYMBOLS, ELF_SHA256["elite"])
    complete = False
    unobserved_edges = None
    observed_window = None
    digest = csv_digest.hexdigest()
    if not isinstance(trace_summary, dict):
        incomplete.append("trace summary absent; requested-window coverage and watch metadata are unverified")
    else:
        window = trace_summary.get("window", {})
        observations = trace_summary.get("scene_observations", {})
        if not isinstance(window, dict):
            window = {}
        if not isinstance(observations, dict):
            observations = {}
        try:
            observed_window = _window(window.get("start_ms"), window.get("end_ms"), "trace summary")
        except ValueError as error:
            incomplete.append(str(error))
        if observed_window != wanted_window:
            incomplete.append("trace observed window does not match the requested observation interval")
        if trace_summary.get("per_half_csv_sha256") != digest:
            incomplete.append("trace summary CSV SHA256 is absent or does not match the parsed CSV bytes")
        if (trace_summary.get("status") != "OBSERVED"
                or trace_summary.get("firmware_source") != FIRMWARE_SOURCE
                or window.get("completed_halves") != rows or window.get("coherent_halves") != rows):
            incomplete.append("trace summary completeness/coherent-half count does not match CSV")
        fields = observations.get("fields")
        if fields != expected_metadata and fields != {**expected_metadata, **ACTIVITY_METADATA}:
            incomplete.append("trace watch metadata does not match the verified target layout")
        elif activity_presence != {"scene_808_active" in fields}:
            incomplete.append("trace watch metadata and CSV boundary field sets disagree")
        edges = window.get("dma_edges")
        if type(edges) is not int or edges < rows:
            incomplete.append("trace DMA edge count is invalid")
        else:
            unobserved_edges = edges - rows
        complete = not incomplete
    match = bool(rows and matching_rows == rows and complete)
    return {"status": "REJECTED" if failing_boundaries else "OBSERVED_MATCH" if match else "INCOMPLETE",
            "required_state_matches_at_all_observed_boundaries": match,
            "owner_scene_exact": False, "evidence_scope": SCOPE,
            "representative_scene_qualified": False,
            "808_live_inactivity": "UNVERIFIED",
            "declared_window": declared_window, "observed_window": observed_window,
            "requested_observation_window": wanted_window, "observation_window_override": override,
            "partial_duration": observed_window != declared_window,
            "layout_receipt": LAYOUT_RECEIPT, "csv_sha256": digest,
            "rows": rows, "coherent_rows": coherent_rows, "matching_rows": matching_rows,
            "checked_boundaries": boundaries, "failing_boundaries": failing_boundaries,
            "trace_complete": complete, "unobserved_dma_edges": unobserved_edges,
            "continuous_state_between_boundaries": "UNVERIFIED", "audibility": "UNVERIFIED",
            "failures": failures, "incomplete_reasons": incomplete, "limitations": LIMITATIONS}
