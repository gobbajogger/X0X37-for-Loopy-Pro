#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import chicago_au_map

ROOT = Path(__file__).resolve().parents[2]


def main() -> int:
    presets = json.loads((ROOT / "assets/factory_banks/chicago.json").read_text())["presets"]
    m = chicago_au_map.preset_from_factory_bank(presets, 0, arp_off=True)
    assert m["bass303"][5] == chicago_au_map._pot_from_byte(chicago_au_map._byte_from_au(0.24409449))
    assert m["chi"]["arp_switch"] == 0
    # Env ADSR dual-drives filter EG (FEnv ADSR) — Acid Lab 1.
    assert m["bass303"][15] == 1, m["bass303"][15]
    assert m["bass303"][12] == m["bass303"][19]
    assert m["bass303"][3] == m["bass303"][20]
    assert m["bass303"][13] == m["bass303"][21]
    assert m["bass303"][14] == m["bass303"][22]
    # Hot Tube / One Hit Wonder / The Visitor dump Amp Level 0 — floor to Init.
    init_vol = chicago_au_map._pot_from_byte(201, 127)
    for idx, name in ((33, "Hot Tube"), (47, "One Hit Wonder"), (71, "The Visitor")):
        assert presets[idx]["name"] == name
        assert chicago_au_map._byte_from_au(presets[idx]["params"][12]) == 0
        row = chicago_au_map.preset_from_factory_bank(presets, idx, arp_off=True)
        assert row["bass303"][7] == init_vol, (name, row["bass303"][7], init_vol)
        assert row["bass303"][15] == 1
    # Non-zero Amp Level still maps linearly (BearHug).
    bear = chicago_au_map.preset_from_factory_bank(presets, 5, arp_off=True)
    assert bear["bass303"][7] == chicago_au_map._pot_from_byte(
        chicago_au_map._byte_from_au(presets[5]["params"][12]), 127
    )
    # Alkaline Bass: Env mode + sustain — filter sustain mirrors amp.
    alkaline = chicago_au_map.preset_from_factory_bank(presets, 2, arp_off=True)
    assert presets[2]["name"] == "Alkaline Bass"
    assert alkaline["bass303"][16] == 0  # Amp Mode Env
    assert alkaline["bass303"][13] == alkaline["bass303"][21] > 0
    print("ok chicago_au_map")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
