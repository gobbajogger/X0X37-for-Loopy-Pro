#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The LCD clip colour table is the Akai palette as plain RGB565 (BEHAVIOR.md CLP-05), with no hand values.

Independent of tools/apc_pad_colors.py and tools/apc_colour_model.py: this test reads
tools/apc40mk2_akai_protocol_v1.2.csv (Akai APC40 Mk2 Communications Protocol v1.2, table "RGB LEDs")
itself and compares all 128 entries of firmware/src/app/apc40_lcd565.h (APC40_LCD565).
Rule (the same as the generator): round to nearest, each channel on its own:
  R5 = (R * 31 + 127) // 255,  G6 = (G * 63 + 127) // 255,  B5 = (B * 31 + 127) // 255,
  RGB565 = R5 << 11 | G6 << 5 | B5.   (No gamma, no floor, no lift.)
The quick check (tools/quickcheck.sh) picks this test when either file changes: it names both."""
import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CSV = ROOT / "tools/apc40mk2_akai_protocol_v1.2.csv"
HDR = ROOT / "firmware/src/app/apc40_lcd565.h"

rows = list(csv.DictReader(CSV.read_text().splitlines()))
assert [int(r["index"]) for r in rows] == list(range(128)), "the CSV must have the indexes 0-127 in order"
want = []
for r in rows:
    rgb = int(r["hex"].lstrip("#"), 16)
    R, G, B = int(r["r"]), int(r["g"]), int(r["b"])
    assert (R, G, B) == (rgb >> 16, (rgb >> 8) & 255, rgb & 255), f"CSV row {r['index']}: hex and r, g, b differ"
    want.append((R * 31 + 127) // 255 << 11 | (G * 63 + 127) // 255 << 5 | (B * 31 + 127) // 255)

m = re.search(r"APC40_LCD565\[128\]\s*=\s*\{(.*?)\};", HDR.read_text(), re.S)
got = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{1,4})\b", m.group(1))] if m else []
assert len(got) == 128, f"APC40_LCD565 must have 128 entries ({len(got)})"
bad = [(v, rows[v]["hex"], got[v], want[v]) for v in range(128) if got[v] != want[v]]
for v, h, g, w in bad:
    print(f"FAIL velocity {v} {h}: header 0x{g:04X}, plain RGB565 0x{w:04X}")
print(f"apc40_lcd565_csv_test: {'ok' if not bad else str(len(bad)) + ' FAIL'} (128 entries)")
sys.exit(1 if bad else 0)
