#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generate Chicago AU scenario battery from assets/factory_banks/chicago.json."""
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "tests" / "scenarios" / "chicago_au"
BANK = ROOT / "assets" / "factory_banks" / "chicago.json"

import sys

sys.path.insert(0, str(ROOT / "tools"))
import au_compare_battery  # noqa: E402


def slugify(name: str) -> str:
    s = name.lower().strip()
    s = re.sub(r"[^a-z0-9]+", "_", s)
    s = re.sub(r"_+", "_", s).strip("_")
    return s or "preset"


def nominal_hz(midi: int) -> float:
    return 440.0 * (2.0 ** ((midi - 69) / 12.0))


def main() -> int:
    doc = json.loads(BANK.read_text())
    presets = doc["presets"]
    OUT.mkdir(parents=True, exist_ok=True)
    for path in list(OUT.glob("chicago_au_*.json")):
        path.unlink()
    for path in list(OUT.glob("chicago_au_*.x0x")):
        path.unlink()

    battery = {"schema": 1, "sampleRate": 48000, "scenarios": []}
    used: set[str] = set()
    for pidx, preset in enumerate(presets):
        base = slugify(preset["name"])
        slug = base
        n = 2
        while slug in used:
            slug = f"{base}_{n}"
            n += 1
        used.add(slug)
        midi = 48
        scen = {
            "id": slug,
            "tags": ["factory_preset", "steady_voiced"],
            "bpm": au_compare_battery.compare_bpm(),
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
            "factoryPresetName": preset["name"],
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
    print(f"ok gen_chicago_au_battery: {len(presets)} scenarios -> {OUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
