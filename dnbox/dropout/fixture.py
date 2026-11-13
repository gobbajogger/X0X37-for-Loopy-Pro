#!/usr/bin/env python3
"""Build an emulator-only FSMP slot from a complete, hashed factory loop.

Formats: firmware94197fc8 tools/break_pack.py and firmware/src/app/slots.c.
No device transport, firmware rebuild, resampling or ADPCM re-encoding.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

FIRMWARE_SOURCE = "94197fc839b00aff47b98ca02f3c6396e4a6eabf"
RUNNER_SOURCE = "31157d25caf7284080cdb5d86ab7e8e438120fd9"
FACTORY_PACK_SHA256 = "cbfe682a9c69b51dac4ee96db7ab4605c8ecdb2839230efcc5445aa0a8fe199e"
FLASH_BYTES = 1 << 20
SLOTS = {1: (0xC2000, 0x14000), 2: (0xD6000, 0x1E000), 3: (0xB3000, 0x8000)}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_pack(data: bytes) -> tuple[int, list[dict]]:
    """Strict whole-file parser; never accepts a valid prefix with extra data."""
    if len(data) < 20 or data[:8] != b"X0XBRKPK":
        raise ValueError("not an X0XBRKPK pack")
    version, count, rate, crc = struct.unpack_from("<HHII", data, 8)
    if version != 1 or not 1 <= count <= 8 or rate not in (22050, 44100):
        raise ValueError("unsupported pack version, count or sample rate")
    loops, pos = [], 20
    for _ in range(count):
        if pos + 20 > len(data):
            raise ValueError("truncated loop header")
        raw_name = data[pos:pos + 8]
        name = raw_name.split(b"\0", 1)[0].decode("ascii", "strict")
        samples, bars, size = struct.unpack_from("<IB3xI", data, pos + 8)
        pos += 20
        if samples < 64 or size != (samples + 1) // 2 or bars not in (1, 2, 4, 8):
            raise ValueError("invalid complete-loop length or bars")
        if pos + size > len(data):
            raise ValueError("truncated loop ADPCM")
        loops.append(dict(name=name, raw_name=raw_name, samples=samples, bars=bars,
                          adpcm=data[pos:pos + size]))
        pos += size
    if pos != len(data):
        raise ValueError("trailing bytes after pack")
    if zlib.crc32(data[20:]) != crc:
        raise ValueError("pack CRC mismatch")
    if len({x["name"] for x in loops}) != len(loops):
        raise ValueError("ambiguous duplicate loop names")
    return rate, loops


def inferred_bars(samples: int, rate: int) -> int:
    # Mirrors the user-slot selection rule at engine.c2960; no pack-bar promise.
    seconds, best, selected = samples / rate, float("inf"), 1
    for bars in (1, 2, 4, 8):
        bpm = bars * 240 / seconds
        distance = abs(bpm - 120)
        if 80 <= bpm <= 180 and distance < best:
            best, selected = distance, bars
    return selected


def make_fixture(base: bytes, pack: bytes, name: str, slot: int) -> tuple[bytes, dict]:
    if len(base) != FLASH_BYTES:
        raise ValueError("base flash must be exactly 1048576 bytes")
    if slot not in SLOTS:
        raise ValueError("SMK sample slot must be 1, 2 or 3")
    for other, (start, _) in SLOTS.items():
        if other != slot and base[start:start + 4] == b"FSMP":
            # Even a corrupt existing header is rejected: this fixture promises
            # one possible user-slot source, not repair of an arbitrary dump.
            raise ValueError(f"other sample slot {other} has an FSMP header; use a fresh base")
    rate, loops = read_pack(pack)
    chosen = next((x for x in loops if x["name"] == name), None)
    if chosen is None:
        raise ValueError(f"loop {name!r} is absent from the pack")
    offset, capacity = SLOTS[slot]
    adpcm = chosen["adpcm"]
    if len(adpcm) > capacity - 512:
        raise ValueError("complete loop exceeds selected slot capacity")
    # Firmware and sampleio.py use root note 60, initial predictor/index 0.
    header = struct.pack("<IHBB8sII8x", 0x504D5346, 1, 1, 0, chosen["raw_name"],
                         len(adpcm), zlib.crc32(adpcm))
    zone = struct.pack("<5I2h4B", 0, chosen["samples"], 0, chosen["samples"],
                       rate * 65536 // 44100, 60 * 16, 0, 0, 0, 127, 1)
    payload = (header + zone).ljust(512, b"\0") + adpcm
    payload = payload.ljust(capacity, b"\xff")
    result = base[:offset] + payload + base[offset + capacity:]
    report = {
        "schema": 1, "status": "PREPARED_NOT_RUN", "owner_scene_exact": False,
        "firmware_source": FIRMWARE_SOURCE, "runner_source": RUNNER_SOURCE,
        "base_flash_sha256": sha256(base), "flash_sha256": sha256(result),
        "unchanged_outside_slot": True,
        "other_sample_slots_with_fsmp_headers": [],
        "sample": {"pack_sha256": sha256(pack), "name": name, "slot": slot,
                   "offset": offset, "slot_bytes": capacity, "slot_sha256": sha256(payload),
                   "adpcm_sha256": sha256(adpcm), "adpcm_bytes": len(adpcm),
                   "samples": chosen["samples"], "rate": rate, "pack_bars": chosen["bars"],
                   "user_slot_bars_inferred": inferred_bars(chosen["samples"], rate)},
        "runtime_evidence": {"loaded": "NOT_RUN", "break_mix_contribution": "NOT_RUN",
                             "audibility": "NOT_RUN", "deadline": "NOT_RUN"},
        "owner_unknown": ["installed_package_hash", "installed_model", "tempo", "patches",
                          "fx", "break_sample", "arp_settings", "held_notes", "velocities"],
    }
    return result, report


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--flash", type=Path, required=True, help="immutable 1 MiB emulator flash")
    p.add_argument("--pack", type=Path, required=True)
    p.add_argument("--pack-sha256", default=FACTORY_PACK_SHA256)
    p.add_argument("--loop", default="CRAMEN")
    p.add_argument("--slot", type=int, choices=sorted(SLOTS), default=1)
    p.add_argument("--out", type=Path, required=True, help="new output directory")
    a = p.parse_args()
    try:
        pack = a.pack.read_bytes()
        if sha256(pack) != a.pack_sha256:
            raise ValueError("input pack SHA256 does not match the explicit pin")
        result, report = make_fixture(a.flash.read_bytes(), pack, a.loop, a.slot)
        a.out.mkdir(parents=True, exist_ok=False)
        (a.out / "fixture.flash").write_bytes(result)
        (a.out / "fixture.json").write_text(json.dumps(report, indent=2) + "\n")
    except (OSError, ValueError) as e:
        p.exit(2, f"fixture: {e}\n")
    print(json.dumps({"status": report["status"], "manifest": str(a.out / "fixture.json")}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
