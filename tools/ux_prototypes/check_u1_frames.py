#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Focused design-frame bounds check; never a firmware UI or audio qualification.
Run from repository root: python3 tools/ux_prototypes/check_u1_frames.py
"""
import importlib.util
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
MODULE = ROOT / "tools/ux_prototypes/render_u1_frames.py"
spec = importlib.util.spec_from_file_location("u1_study_renderer", MODULE)
assert spec and spec.loader
ui = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ui)
old_label = ui.label
calls = []
overflows = []

def checked_label(img, x, y, text, face="S", color=None, limit=238):
    first, last, height, glyphs = ui.FONTS[face]
    end = x
    for char in text:
        code = ord(char)
        if char.islower() and code > last:
            code -= 32
        if not first <= code <= last:
            code = ord("?")
        end += glyphs[code - first][0]
    calls.append((text, x, y, end))
    if x < 0 or y < 0 or y + height > 240 or end > limit:
        overflows.append((text, x, y, end, limit))
    return old_label(img, x, y, text, face,
                     ui.WHITE if color is None else color, limit)

ui.label = checked_label
ui.main()
images = sorted(ui.OUT.glob("*.png"))
assert len(images) == 6, [p.name for p in images]
for path in images:
    with Image.open(path) as im:
        assert im.size == (240, 240), (path, im.size)
        assert im.mode == "RGB", (path, im.mode)
assert not overflows, f"Text glyph advance outside allocated zone: {overflows}"
assert len(calls) >= 90, len(calls)
for required in ("DnBOX", "Cmin7", "C/E", "C4 Eb4 G4 Bb4", "E4 G4 C5",
                 "HELD: C4 E4 G4", "IMPORT: VERIFY FORMAT"):
    assert any(t == required for t, *_ in calls), required
print(f"U1 DESIGN STUDY PASS: 6 native 240x240 RGB frames, {len(calls)} text regions, 0 text bounds failures.")
print("NOT PROOF: actual firmware UI frames, display hardware, audio or latency.")
