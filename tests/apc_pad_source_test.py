#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Violet proof test: replay the raw clip velocity and channel from an INLOG log and compare the
live pad table (APC40_PAD_OWNER in app/apc40_pad_owner.h; APC40_RGB in main_fm1.c before 2026-10-17)
with the Akai palette (tools/apc40mk2_akai_protocol_v1.2.csv).

For each velocity that Loopy sent in the log, the table entry must be the Akai colour. The channel
selects the state only (protocol v1.2 "RGB LEDs Type": 0 solid, 1-5 one-shot, 6-10 pulse, 11-15
blink), so it must not change the colour. The test also lists every table entry that is not
the Akai colour (also velocities that are not in the log).
Usage: tests/apc_pad_source_test.py [FIXTURE ...]   (default: tests/fixtures/inlog_*_clips.txt)
OBSOLETE as a pass/fail test, evidence only, not in tests/run_tests.sh. Since 2026-10-17 the live pad
table is the owner-calibrated table (owner export tools/padcal/pads-owner-20261017.txt, sha256
54ecc6f8..., BEHAVIOR.md CLP-13). Its entries are pad LED levels that the owner matched by eye, not
Akai colours, so this test fails on nearly every entry by design. tests/apc40_pad_owner_test.py checks
the live table. History: decision 2026-10-16 23:10 (Orchestrator) kept the old table with 10 WS2812
corrections (48-51, 69, 80, 81, 93, 94, 116) and named the pad calibration mode as the fix path.
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import apc_colour_model as m  # noqa: E402

import inlog_fmt  # noqa: E402
live = inlog_fmt.palette()
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
