#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read the reply of editor command 38 INLOG (SMK-37, a76) and print it in plain words.

The reply: the entry count, fm1_ms now, then 2 words for each entry (the oldest first), each word
5 x 7 bits, low bits first. Then the 40 stored clip velocities and the 40 stored clip channels.
Entry word 1 is fm1_ms. Entry word 2 is src << 24 | status << 16 | d1 << 8 | d2.
src: 4 decoded SMC input (MMC command in d1), 0 USB, 1 Bluetooth, 2 Bluetooth link edge (d2 bit 0 link up, bit 1 notify on), 3 Bluetooth pad colour note-on while notify is off (the cell did not change).
tools/ble_autotest.py --diag-only uses this module.
"""
import colorsys
import re
from pathlib import Path

ED_INLOG = 38
PAD_INPUT_ARG = 6
PAD_INPUT_BYTES = 521
CELLS = 40
SRC = ["USB", "BLE", "EDGE", "BLE NO CELL"]
MAIN = Path(__file__).resolve().parents[1] / "firmware/src/app/apc40_pad_owner.h"


def table_rgb(text):
    """The pad table in a source text as 0xRRGGBB words: APC40_PAD_OWNER[128][3] (decimal, since 2026-10-17)
    or APC40_RGB[128] (hex, older builds). [] if none."""
    m = re.search(r"APC40_PAD_OWNER\[128\]\[3\]\s*=\s*\{(.*?)\};", text, re.S)
    if m:
        return [int(r) << 16 | int(g) << 8 | int(b) for r, g, b in re.findall(r"\{(\d+),(\d+),(\d+)\}", m.group(1))]
    m = re.search(r"APC40_RGB\[128\]\s*=\s*\{(.*?)\};", text, re.S)
    return [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{6})", m.group(1))] if m else []


def palette(path=MAIN):
    """The pad table of the firmware (the colour that the pads draw, level / 2 when lit), or None."""
    try:
        pal = table_rgb(Path(path).read_text())
    except OSError:
        return None
    return pal if len(pal) == 128 else None


def colour_name(rgb):
    r, g, b = rgb >> 16, (rgb >> 8) & 0xFF, rgb & 0xFF
    mx, mn = max(r, g, b), min(r, g, b)
    if mx == 0:
        return "off"
    dim = "dim " if mx < 0x60 else ""
    if (mx - mn) / mx < 0.2:
        return dim + ("white" if mx > 0xC0 else "grey")
    h = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)[0] * 360
    for top, name in ((15, "red"), (40, "orange"), (70, "yellow"), (100, "lime"), (150, "green"),
                      (175, "teal"), (200, "cyan"), (225, "azure"), (255, "blue"), (275, "violet"),
                      (295, "purple"), (320, "magenta"), (345, "pink"), (361, "red")):
        if h < top:
            return dim + name
    return dim + "red"


def mode(ch):
    return "steady" if ch == 0 else "one-shot" if ch <= 5 else "pulse" if ch <= 10 else "blink"


def parse(r):
    """r: the reply bytes after the command byte. Return (now_ms, entries, velocities, channels)."""
    if not r:
        return None
    c = r[0]
    words = []
    for k in range(1 + 2 * c):
        b = r[1 + 5 * k:6 + 5 * k]
        if len(b) < 5:
            return None
        words.append(b[0] | b[1] << 7 | b[2] << 14 | b[3] << 21 | (b[4] & 15) << 28)
    cells = r[1 + 5 * (1 + 2 * c):]
    if len(cells) < 2 * CELLS:
        return None
    ents = [(words[1 + 2 * i], words[2 + 2 * i]) for i in range(c)]
    return words[0], ents, list(cells[:CELLS]), list(cells[CELLS:2 * CELLS])


def vel_text(v, pal):
    if v == 0:
        return "vel 0 (off)"
    if not pal:
        return f"vel {v} (0x{v:02X})"
    return f"vel {v} (0x{v:02X}, {colour_name(pal[v])}, pad table {pal[v]:06X})"


def entry_text(now, ms, w, pal):
    src, st, d1, d2 = w >> 24, (w >> 16) & 0xFF, (w >> 8) & 0x7F, w & 0x7F
    d = (ms - now) & 0xFFFFFFFF                  # fm1_ms wraps at 2^32
    if d >= 1 << 31:
        d -= 1 << 32
    t = f"{d / 1000:+9.3f} s"
    if src == 2:
        return f"{t}  EDGE         link {'up' if d2 & 1 else 'down'}, notify {'on' if d2 & 2 else 'off'}"
    if src == 4:
        if st == 0xF0:
            return f"{t}  SMC          MMC command {d1}"
        return f"{t}  SMC          MIDI {st:02X} {d1:02X} {d2:02X}"
    kind = "note on " if st & 0xF0 == 0x90 and d2 else "note off"
    ch = st & 0x0F
    where = f"cell row {d1 // 8} col {d1 % 8}" if d1 < CELLS else "LED note"
    v = d2 if kind == "note on " else 0
    return (f"{t}  {SRC[src % 4]:11s}  {kind} ch {ch + 1:2d} ({mode(ch)}) note {d1:3d} ({where}) "
            + vel_text(v, pal))


def text(p, pal=None):
    if p is None:
        return ["INLOG: no reply or a short reply (firmware before a76?)"]
    now, ents, vel, ch = p
    out = [f"INLOG: {len(ents)} entries, the oldest first (time before the read, fm1_ms now {now})"]
    out += ["  " + entry_text(now, ms, w, pal) for ms, w in ents]
    out.append("Stored clip cells (APC row 4 = top Loopy row; velocity/channel; '.' = off):")
    for row in range(4, -1, -1):
        out.append(f"  row {row}: " + " ".join(f"{vel[row * 8 + c]:3d}/{ch[row * 8 + c]:<2d}" if vel[row * 8 + c] else "   .  "
                                             for c in range(8)))
    lit = sorted({v for v in vel if v})
    if lit:
        out.append("Colours in the cells: " + "; ".join(vel_text(v, pal) for v in lit))
    return out


def parse_pads(r):
    """Reply to INLOG arg 1. Return (dirty, shadow, sent): 16 GRB words each, hardware pad order."""
    if not r or r[0] != 1 or len(r) < 2 + 4 * 32:
        return None
    w = [r[2 + 4 * k] | r[3 + 4 * k] << 7 | r[4 + 4 * k] << 14 | r[5 + 4 * k] << 21 for k in range(32)]
    return r[1], w[:16], w[16:]


def parse_pad_inputs(r):
    """INLOG 6 v1. Decode strictly; legacy fallback, short and noncanonical replies are not data."""
    if not r or any(not isinstance(b, int) or not 0 <= b <= 127 for b in r):
        return None
    if list(r[:3]) == [PAD_INPUT_ARG, 1, 1] and len(r) == 3:
        return {"version": 1, "status": "busy"}
    if len(r) != PAD_INPUT_BYTES or list(r[:4]) != [PAD_INPUT_ARG, 1, 0, 16] or r[4] > 3:
        return None
    at = 8

    def take(bits):
        nonlocal at
        n = (bits + 6) // 7
        v = sum(r[at + k] << (7 * k) for k in range(n))
        at += n
        if v >= 1 << bits:
            raise ValueError("noncanonical word")
        return v

    try:
        d = {"version": 1, "status": "snapshot", "grid": bool(r[4] & 1), "piano": bool(r[4] & 2),
             "on_delta": r[5], "off_delta": r[6], "peak_samples": r[7]}
        for key in ("now_ms", "frames", "adc_ok", "adc_miss", "pnotes"):
            d[key] = take(32)
        d["mux_c"] = [take(16) for _ in range(8)]
        d["pads"] = []
        for i in range(16):
            p = {"pad": i}
            for key in ("raw", "base", "peak"):
                p[key] = take(16)
            p["state"] = take(8)
            for key in ("velocity", "pressure", "note_plus_one"):
                p[key] = take(7)
            p["curve"] = [take(16) for _ in range(5)]
            d["pads"].append(p)
        return d if at == len(r) else None
    except ValueError:
        return None


def pad_inputs_text(d):
    if d is None:
        return ["PAD INPUTS: unsupported, missing or malformed reply; no sensor conclusion."]
    if d["status"] == "busy":
        return ["PAD INPUTS: scan crossed all three snapshot attempts; busy, no measurements."]
    out = [f"PAD INPUTS: latest mux samples at {d['now_ms']} ms; frames {d['frames']}, "
           f"ADC ok {d['adc_ok']}, missed {d['adc_miss']}; pnotes 0x{d['pnotes']:08x}",
           f"Hit above base + {d['on_delta']}; release below base + {d['off_delta']}; "
           f"peak window {d['peak_samples']} samples; grid {d['grid']}, piano {d['piano']}",
           "Mux C: faders 1-4 (raw), wheels (filtered), pedal, battery: " + str(d["mux_c"]),
           "Hardware pads 0-7 top row, 8-15 bottom row, left to right:"]
    for p in d["pads"]:
        out.append(f"  pad {p['pad']:2d}: raw {p['raw']:4d} base {p['base']:4d} "
                   f"delta {p['raw'] - p['base']:+5d} peak {p['peak']:4d} state 0x{p['state']:02x} "
                   f"vel {p['velocity']:3d} pressure {p['pressure']:3d} "
                   f"note+1 {p['note_plus_one']:2d} curve {p['curve']}")
    out.append("Peak/velocity are the last strike, not a press counter. Curves are active RAM values; "
               "factory/default origin is not reported. A snapshot alone does not diagnose the hardware.")
    return out


PADCAL_ARGS = (2, 3, 4, 5)   # CLP-79: the pad calibration export, 32 entries each


def parse_padcal(r, arg):
    """Reply to INLOG arg 2-5: the argument, then R, G, B of 32 entries and 4 bytes of ok bits, each
    2 x 7 bits. Return 32 (velocity, R, G, B, ok) tuples, or None."""
    if not r or r[0] != arg or len(r) < 1 + 2 * (96 + 4):
        return None
    b = [r[1 + 2 * k] | r[2 + 2 * k] << 7 for k in range(100)]
    if max(b) > 255:
        return None
    return [((arg - 2) * 32 + k, b[3 * k], b[3 * k + 1], b[3 * k + 2], (b[96 + k // 8] >> (k % 8)) & 1)
            for k in range(32)]


def parse_padcal_tl(r):
    """The orientation correction after the ok bits (CLP-79): the pad index found top-left, or None."""
    if not r or len(r) < 1 + 2 * 101:
        return None
    return r[201] | r[202] << 7


def padcal_lines(entries, tl=None):
    """The export file for tools/apc_pad_colors.py --pad-override: one line for each entry, then the
    orientation correction ("padcal_orient TL", 0 = the pad order as assumed)."""
    out = [f"padcal {v} {r} {g} {b} {'ok' if ok else '-'}" for v, r, g, b, ok in entries]
    return out + ([] if tl is None else [f"padcal_orient {tl}"])


def rgb_of_grb(v):
    return ((v >> 8) & 0xFF) << 16 | ((v >> 16) & 0xFF) << 8 | (v & 0xFF)


def pads_text(pp, cells=None):
    if pp is None:
        return ["PADS: no reply (firmware before a76?)"]
    dirty, shadow, sent = pp
    out = [f"PADS: the LED output (hardware pads 0-7 top row, 8-15 bottom row; R,G,B; "
           f"a frame waits to go out: {'yes' if dirty else 'no'})"]
    for p in range(16):
        a, b = rgb_of_grb(shadow[p]), rgb_of_grb(sent[p])
        s = f"  pad {p:2d}: to send {a >> 16:3d},{(a >> 8) & 255:3d},{a & 255:3d} ({colour_name(a)})"
        s += f"  sent {b >> 16:3d},{(b >> 8) & 255:3d},{b & 255:3d} ({colour_name(b)})"
        if a != b and not dirty:
            s += "  MISMATCH: the pad shows a colour that the firmware did not ask for"
        out.append(s)
    lit = sum(1 for v in sent if v)
    if cells is not None and not any(cells) and lit:
        out.append(f"  WARNING: all clip cells are off, but {lit} pads show a colour")
    return out
