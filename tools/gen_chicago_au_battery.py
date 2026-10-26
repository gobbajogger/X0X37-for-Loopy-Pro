#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generate Chicago AU scenario battery from assets/factory_banks/chicago.json."""
from __future__ import annotations

import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "tests" / "scenarios" / "chicago_au"
BANK = ROOT / "assets" / "factory_banks" / "chicago.json"

# 10 steady-voiced rows: diverse factory presets (arp forced off in harness).
PRESET_ROWS = [
    ("acid_lab_1", 0),
    ("acid_lab_2", 1),
    ("bass_simple", 4),
    ("bearhug", 5),
    ("bite_bass", 6),
    ("square_cut", 7),  # Bite Me! — heavy bite
    ("gnaw_line", 12),  # preset index picked below after scan
    ("open_saw", 20),
    ("env_sustain", 25),
    ("filter_peak", 30),
]

# Resolve named presets when index placeholder used
def _index_by_name(presets, name: str, fallback: int) -> int:
    for i, p in enumerate(presets):
        if p["name"].lower().replace(" ", "_") == name.replace(" ", "_").lower():
            return i
        if name.lower() in p["name"].lower():
            return i
    return fallback


def nominal_hz(midi: int) -> float:
    return 440.0 * (2.0 ** ((midi - 69) / 12.0))


def main() -> int:
    doc = json.loads(BANK.read_text())
    presets = doc["presets"]
    OUT.mkdir(parents=True, exist_ok=True)
    rows = list(PRESET_ROWS)
    rows[6] = ("gnaw_line", _index_by_name(presets, "Gnaw", 12))

    battery = {"schema": 1, "sampleRate": 48000, "scenarios": []}
    for slug, pidx in rows:
        if pidx >= len(presets):
            raise SystemExit(f"preset index {pidx} out of range for {slug}")
        midi = 48
        scen = {
            "id": slug,
            "tags": ["factory_preset", "steady_voiced"],
            "sampleRate": 48000,
            "frames": 48000,
            "noteOnFrame": 4800,
            "noteOffFrame": 24000,
            "steadyWindowStart": 4800,
            "steadyWindowEnd": 19200,
            "midiNote": midi,
            "velocity": 80,
            "nominalHz": nominal_hz(midi),
            "refWav": f"{slug}.wav",
            "factoryPresetIndex": pidx,
            "factoryPresetName": presets[pidx]["name"],
            "arpOff": True,
        }
        path = OUT / f"chicago_au_{slug}.json"
        path.write_text(json.dumps(scen, indent=2) + "\n")
        x0x = OUT / f"chicago_au_{slug}.x0x"
        x0x.write_text(
            f"# Chicago AU offline scenario {slug} — manifest: chicago_au_{slug}.json\n"
        )
        battery["scenarios"].append({"id": slug, "manifest": path.name})
    (OUT / "battery.json").write_text(json.dumps(battery, indent=2) + "\n")
    print(f"ok gen_chicago_au_battery: {len(rows)} scenarios -> {OUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
