#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Violet proof test: replay the raw clip velocity and channel from an INLOG log and compare the
live pad table (APC40_RGB in main_fm1.c) with the Akai palette (tools/apc40mk2_akai_protocol_v1.2.csv).

For each velocity that Loopy sent in the log, the table entry must be the Akai colour. The channel
selects the state only (protocol v1.2 "RGB LEDs Type": 0 solid, 1-5 one-shot, 6-10 pulse, 11-15
blink), so it must not change the colour. The test also lists every table entry that is not
the Akai colour (also velocities that are not in the log).
Usage: tests/apc_pad_source_test.py [FIXTURE ...]   (default: tests/fixtures/inlog_*_clips.txt)
Evidence file, not in tests/run_tests.sh: it fails on the live table (velocity 81: CC00FF, Akai 7A00FF).
Decision 2026-10-16 23:10 (Orchestrator): the live pad table does not change. The 10 entries that are
not the Akai colour (48-51, 69, 80, 81, 93, 94, 116) are WS2812 corrections after an owner VIOLET FAIL.
The raw Akai colour can look blue on the pads. The pad calibration mode (GLOBAL > SYSTEM > PAD COLOURS)
is the fix path: its export, not this test, changes the pad table.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import apc_colour_model as m  # noqa: E402

main = (ROOT / "firmware/src/app/main_fm1.c").read_text()
t = re.search(r"APC40_RGB\[128\]\s*=\s*\{(.*?)\};", main, re.S)
live = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{6})", t.group(1))]
akai = [e["apc"] for e in m.load_source()]
STATE = ["solid"] + ["one-shot"] * 5 + ["pulse"] * 5 + ["blink"] * 5
fails = 0
fixtures = [Path(a) for a in sys.argv[1:]] or sorted((ROOT / "tests/fixtures").glob("inlog_*_clips.txt"))
for fx in fixtures:
    seen = {}
    for line in fx.read_text().splitlines():
        if line.startswith("state "):
            head = line.split(" | ")[0].split()
            for note, c in enumerate(head[2:]):
                v, ch = (int(x) for x in c.split("/"))
                if v:
                    seen.setdefault((v, ch), set()).add(note)
    print(f"replay {fx.name}: {len(seen)} velocity/channel pairs")
    for (v, ch), notes in sorted(seen.items()):
        ok = live[v] == akai[v]
        fails += not ok
        print(f"{'PASS' if ok else 'FAIL'} vel {v:3d} ch {ch:2d} ({STATE[ch]}) notes {sorted(notes)}: "
              f"table {live[v]:06X}, Akai {akai[v]:06X}")
diff = [v for v in range(128) if live[v] != akai[v]]
print(f"table entries not equal to Akai: {len(diff)}")
for v in diff:
    print(f"  vel {v:3d}: table {live[v]:06X}, Akai {akai[v]:06X}")
print("apc_pad_source_test: " + ("ok" if not fails else f"{fails} FAIL"))
sys.exit(1 if fails else 0)
