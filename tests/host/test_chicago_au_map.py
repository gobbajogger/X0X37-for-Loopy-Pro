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
    print("ok chicago_au_map")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
