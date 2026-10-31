#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""tools/inlog_fmt.py: decode an editor command 38 INLOG reply and print it in plain words."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import inlog_fmt as f  # noqa: E402


def w5(v):
    return [(v >> s) & 0x7F for s in (0, 7, 14, 21)] + [(v >> 28) & 0x0F]


def reply(now, ents, vel, ch):
    r = [len(ents)] + w5(now)
    for ms, w in ents:
        r += w5(ms) + w5(w)
    return r + vel + ch


vel = [0] * 40
ch = [0] * 40
vel[0x20], vel[0x21], ch[0x21] = 69, 0x0B, 14
ents = [(1000, 2 << 24 | 3), (1500, 1 << 24 | 0x90 << 16 | 0x20 << 8 | 69),
        (1510, 1 << 24 | 0x9E << 16 | 0x21 << 8 | 0x0B), (1600, 3 << 24 | 0x80 << 16 | 0x20 << 8 | 0x7F),
        (1700, 0 << 24 | 0x90 << 16 | 0x05 << 8 | 0x45)]
p = f.parse(reply(2000, ents, vel, ch))
assert p and p[0] == 2000 and p[1] == ents and p[2] == vel and p[3] == ch
pal = f.palette()
assert pal and len(pal) == 128
t = f.text(p, pal)
s = "\n".join(t)
print(s)
assert "EDGE         link up, notify on" in t[1]
assert "BLE          note on  ch  1 (steady) note  32 (cell row 4 col 0) vel 69 (0x45, " in t[2]
assert "ch 15 (blink)" in t[3] and "BLE NO CELL  note off" in t[4] and "vel 0 (off)" in t[4]
assert t[5].strip().split()[2] == "USB" and t[1].strip().startswith("-1.000 s")
assert f.parse(reply(5, [(0xFFFFFFFB, 2 << 24)], vel, ch)) and "-0.010 s" in f.text(f.parse(reply(5, [(0xFFFFFFFB, 2 << 24)], vel, ch)))[1]
assert f.colour_name(0xFF00FF) == "magenta" and f.colour_name(0x0000FF) == "blue"
assert f.colour_name(0x000019) == "dim blue" and f.colour_name(0) == "off" and f.colour_name(0xFFFFFF) == "white"
full = [(3000 + i, 0 << 24 | 0x90 << 16 | (i % 40) << 8 | 1) for i in range(32)]   # a full ring: 32 entries (SMK_INLOG_N)
pf = f.parse(reply(4000, full, vel, ch))
assert pf and pf[1] == full and f.text(pf, pal)[0].startswith("INLOG: 32 entries")
assert len(reply(4000, full, vel, ch)) + 5 + 1 <= 732   # + header and F7: it fits ed_out (editor.c)
old = [(3000 + i, 0 << 24 | 0x90 << 16 | (i % 40) << 8 | 1) for i in range(64)]   # an old 64-entry firmware reply still decodes
assert f.parse(reply(4000, old, vel, ch))[1] == old
assert f.parse([2] + w5(1)) is None                   # a short reply
assert f.text(None)[0].startswith("INLOG: no reply")

def w4(v):
    return [(v >> s) & 0x7F for s in (0, 7, 14, 21)]


shadow = [0] * 16
sent = [0] * 16
shadow[3] = sent[3] = 0x00007F            # GRB: blue 127
sent[5] = 0x007F00                        # GRB: red 127, but the shadow is dark
pp = f.parse_pads([1, 0] + sum((w4(v) for v in shadow + sent), []))
assert pp == (0, shadow, sent)
t2 = f.pads_text(pp, [0] * 40)
print("\n".join(t2))
assert "sent   0,  0,127 (blue)" in t2[4] and "MISMATCH" not in t2[4]
assert "MISMATCH" in t2[6] and "sent 127,  0,  0 (red)" in t2[6]
assert "WARNING: all clip cells are off, but 2 pads show a colour" in t2[-1]
assert f.parse_pads([1, 0, 1]) is None and f.pads_text(None)[0].startswith("PADS: no reply")
print("PASS INLOG reply decode and text")

assert "SMC          MIDI BF 28 64" in f.entry_text(100, 100, 0x04BF2864, None)
assert "SMC          MMC command 24" in f.entry_text(100, 100, 0x04F01800, None)
