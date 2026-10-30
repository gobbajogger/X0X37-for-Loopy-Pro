#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Clip colours: one source (the Akai palette), the colour rules, and a replay of real INLOG data.

The pad values here are the calibration start point (gamma, white gains, floor), not the live
pad table. The live pad table is the owner-calibrated table app/apc40_pad_owner.h
(tests/apc40_pad_owner_test.py).

The committed headers and the tool output are compared in tests/apc40_tables_test.c.
Usage: tests/apc_palette_test.py [FIXTURE ...]   (default: tests/fixtures/inlog_*_clips.txt)
"""
import csv
import hashlib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import apc_colour_model as m  # noqa: E402

fails = 0


def check(ok, text):
    global fails
    if not ok:
        fails += 1
    print(("PASS " if ok else "FAIL ") + text)


src = m.load_source()
pad = [m.pad_level(e) for e in src]       # the calibration start point (app/apc40_pad_default.h)
send = [m.pad_send(p) for p in pad]
lcd = [m.lcd565(e) for e in src]

# 1. The source is the Akai table (protocol v1.2 "RGB LEDs"). Spot values from the document and tonight's capture.
rows = list(csv.DictReader((ROOT / "tools/apc40mk2_akai_protocol_v1.2.csv").read_text().splitlines()))
check(len(rows) == 128 and all(r["source"].endswith("APC40Mk2_Communications_Protocol_v1.2.pdf") for r in rows),
      "source: 128 rows, each cites the Akai APC40 Mk2 protocol v1.2 PDF")
spot = {0: 0x000000, 3: 0xFFFFFF, 5: 0xFF0000, 39: 0x001019, 47: 0x000019, 49: 0x5400FF, 55: 0x190019,
        81: 0x7A00FF, 100: 0x392B00, 127: 0x4B1502}
check(all(src[v]["apc"] == c for v, c in spot.items()), "source: spot values (39, 47, 55 as captured; 81 = 7A00FF)")
check(src[81]["live"] == 0xA34BAD and src[60]["live"] == 0xED4325 and src[59]["live"] is None,
      "source: Live clip colour column for 60-119 only")

# 2. Both tables come from the same source entry: lit in the source <=> lit on the pad and the LCD.
check(all((e["apc"] != 0) == (max(s) > 0) == (c != 0) for e, s, c in zip(src, send, lcd)),
      "pad and LCD: lit exactly where the source is lit (velocity 0 dark)")
check(all(max(s) >= m.PAD_FLOOR_SENT for e, s in zip(src, send) if e["apc"]),
      f"pad: each lit entry sends at least {m.PAD_FLOOR_SENT} (the dim 0x19 entries show)")


def hdiff(a, b):
    return (a - b + 180) % 360 - 180


# 3. Hue: violet, purple and pink never collapse to blue. Pad hue from the LED model (WS2812B primaries).
blue_pad = m.led_hue(send[45])[0]                     # 45 = 0000FF
blue_lcd = m.lch(*m.of565(lcd[45]))[2]
band = [e["v"] for e in src if e["apc"] and m.full_chroma(e["apc"]) > 0.12 and
        275.0 <= m.full_hue(e["apc"]) <= 345.0]       # violet .. pink in OKLCH
DIM = 6                                               # brightest sent level at or below: LED bit depth limits the hue
for v in band:
    hp = m.led_hue(send[v])[0]
    hl = m.lch(*m.of565(lcd[v]))[2]
    bright = max(send[v]) > DIM
    ok = hdiff(hl, blue_lcd) >= 8.0 and (not bright or hdiff(hp, blue_pad) >= 8.0)
    check(ok, f"hue vel {v:3d} {src[v]['apc']:06X}: LCD {hl:5.1f} (blue {blue_lcd:.1f}), "
              f"pad {hp:5.1f} (blue {blue_pad:.1f}){'' if bright else ' (dim: LCD only)'}")
# Hue order: in the blue..pink range, an entry with a larger source hue (10 degrees or more) keeps a larger hue.
# Source hue: for the LCD (plain RGB565) the hue of the source colour, for the pads the hue at full brightness.
def src_hue(v):
    return m.lch(*m.split(src[v]["apc"]))[2]


rng = [e["v"] for e in src if e["apc"] and m.full_chroma(e["apc"]) > 0.12 and 255.0 <= m.full_hue(e["apc"]) <= 345.0]
bad = []
for a in rng:
    for b in rng:
        if src_hue(b) - src_hue(a) >= 10.0 and hdiff(m.lch(*m.of565(lcd[b]))[2], m.lch(*m.of565(lcd[a]))[2]) <= 0:
            bad.append(("LCD", a, b))
        if m.full_hue(src[b]["apc"]) - m.full_hue(src[a]["apc"]) >= 10.0:
            if max(send[a]) > DIM and max(send[b]) > DIM and hdiff(m.led_hue(send[b])[0], m.led_hue(send[a])[0]) <= 0:
                bad.append(("pad", a, b))
check(not bad, f"hue order blue..pink kept on the LCD and the pads ({len(rng)} entries) {bad[:6]}")
# LCD: no large hue drift (owner rule). Plain RGB565 changes a hue only by its bit depth: each hue
# within 10 degrees of the hue of the source colour. The hue at full brightness can be further away
# (the sRGB bias of dark source values): NOTE only.
def lcd_dev(ref):
    return [(e["v"], f"{e['apc']:06X}", round(hdiff(m.lch(*m.of565(c))[2], ref(e["apc"])), 1))
            for e, c in zip(src, lcd) if e["apc"] and m.full_chroma(e["apc"]) > 0.08]


dev = lcd_dev(lambda x: m.lch(*m.split(x))[2])
check(all(abs(d) <= 10.0 for _, _, d in dev), "LCD hue within 10 degrees of the hue of the source colour "
      f"(largest {max(dev, key=lambda x: abs(x[2]))})")
print(f"NOTE LCD hue more than 8 degrees from the hue at full brightness: "
      f"{[x for x in lcd_dev(m.full_hue) if abs(x[2]) > 8.0]}")

# 4. Distinct source colours stay distinct. Report the collisions that the LED bit depth makes.
by_src = {}
for e in src:
    by_src.setdefault(e["apc"], []).append(e["v"])
dups = [vs for vs in by_src.values() if len(vs) > 1]
pad_col, lcd_col = {}, {}
for e in src:
    if e["apc"]:
        pad_col.setdefault(send[e["v"]], set()).add(e["apc"])
        lcd_col.setdefault(lcd[e["v"]], set()).add(e["apc"])
pc = {k: sorted(v) for k, v in pad_col.items() if len(v) > 1}
lc = {k: sorted(v) for k, v in lcd_col.items() if len(v) > 1}
print(f"NOTE source duplicates (the same colour in the Akai table): {dups}")
for k, v in sorted(pc.items()):
    vs = [e["v"] for e in src if e["apc"] in v]
    print(f"NOTE pad collision (LED bit depth): sent {k} for source {' '.join(f'{x:06X}' for x in v)} (velocities {vs})")
# Plain RGB565 keeps 5 bits of blue: 001900 and 001902 (velocities 23 and 27) give the same code.
# Reported to the Orchestrator (2026-10-16). Any other LCD clash fails.
LCD_565_CLASH = {0x00C0: [0x001900, 0x001902]}
for k, v in sorted(lc.items()):
    vs = [e["v"] for e in src if e["apc"] in v]
    print(f"NOTE LCD clash (RGB565 bit depth): {k:04X} for source {' '.join(f'{x:06X}' for x in v)} (velocities {vs})")
check(lc == LCD_565_CLASH, f"LCD: different source colours give different RGB565, except the known 5-bit clash {lc}")
digest = hashlib.sha256(bytes(b for s in send for b in s)).hexdigest()[:12]
print(f"NOTE pad sent table digest {digest}")

# 5. Replay: INLOG states from the device. The pad bytes came from the old table of that build.
fixtures = [Path(a) for a in sys.argv[1:]] or sorted((ROOT / "tests/fixtures").glob("inlog_*_clips.txt"))
for fx in fixtures:
    old, states = None, []
    for line in fx.read_text().splitlines():
        if line.startswith("old_table "):
            old = [int(x, 16) for x in line.split()[1:]]
        elif line.startswith("state "):
            head, pads = line.split(" | ")
            parts = head.split()
            cells = [tuple(int(x) for x in c.split("/")) for c in parts[2:]]
            states.append((parts[1], cells, [tuple(int(x) for x in p.split(",")) for p in pads.split()]))
    check(old is not None and len(old) == 128 and states, f"replay {fx.name}: {len(states)} states")
    vels = set()
    for t, cells, pads in states:
        lit = {v for v, ch in cells if v}
        vels |= lit
        unknown, expect = [], []
        for i, p in enumerate(pads):
            cand = sorted(v for v in lit | {0} if tuple(((old[v] >> s) & 255) * 255 // 510 for s in (16, 8, 0)) == p)
            if not cand:
                unknown.append((i, p))
                continue
            news = {send[v] for v in cand}
            expect.append(f"{i}:{'/'.join(map(str, cand))}->{','.join(map(str, sorted(news)[0]))}")
            if len(news) > 1:
                unknown.append((i, p, "two velocities with the same old colour"))
        check(not unknown, f"replay {t}: each pad colour comes from a stored cell, new pads {' '.join(expect)} {unknown}")
    for v in sorted(vels):
        print(f"NOTE replay velocity {v}: Akai {src[v]['apc']:06X}"
              f"{'' if src[v]['live'] is None else ' (Live ' + format(src[v]['live'], '06X') + ')'}, "
              f"pad old {old[v]:06X} send {tuple(((old[v] >> s) & 255) * 255 // 510 for s in (16, 8, 0))}, "
              f"pad new send {send[v]}, LCD {lcd[v]:04X}")

# 6. The calibration export overrides the rule only for the entries marked ok.
import subprocess  # noqa: E402
import tempfile  # noqa: E402
with tempfile.TemporaryDirectory() as td:
    ov = Path(td) / "export.txt"
    ov.write_text("padcal 81 200 0 255 ok\npadcal 47 0 0 9 -\npadcal 0 1 2 3 ok\nother line\n")
    hdr = Path(td) / "pad.h"
    subprocess.run([sys.executable, str(ROOT / "tools/apc_pad_colors.py"), "--pad-h", str(hdr), "--pad-override", str(ov)],
                   check=True)
    got = [tuple(int(x) for x in t) for t in re.findall(r"\{(\d+),(\d+),(\d+)\}", hdr.read_text())]
    want = list(pad)
    want[81], want[0] = (200, 0, 255), (1, 2, 3)
    check(got == want and "--pad-override (2 entries)" in hdr.read_text(),
          "pad override: the entries marked ok replace the rule, the others stay")

print("apc_palette_test: " + ("ok" if not fails else f"{fails} FAIL"))
sys.exit(1 if fails else 0)
