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
    # Hot Tube / One Hit Wonder / The Visitor dump Amp Level 0 — floor to Init.
    init_vol = chicago_au_map._pot_from_byte(201, 127)
    for idx, name in ((33, "Hot Tube"), (47, "One Hit Wonder"), (71, "The Visitor")):
        assert presets[idx]["name"] == name
        assert chicago_au_map._byte_from_au(presets[idx]["params"][12]) == 0
        row = chicago_au_map.preset_from_factory_bank(presets, idx, arp_off=True)
        assert row["bass303"][7] == init_vol, (name, row["bass303"][7], init_vol)
    # Non-zero Amp Level still maps linearly (BearHug).
    bear = chicago_au_map.preset_from_factory_bank(presets, 5, arp_off=True)
    assert bear["bass303"][7] == chicago_au_map._pot_from_byte(
        chicago_au_map._byte_from_au(presets[5]["params"][12]), 127
    )
    print("ok chicago_au_map")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
