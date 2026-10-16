#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""SMK-37 clip mode: calculate the pad LED and LCD colour for each APC40 mkII velocity.

The script reads APC40_RGB[128] from firmware/src/app/main_fm1.c and applies the
same integer arithmetic as the firmware:

  pads (smk_pads):        led = palette * brightness / 510, brightness 255 (steady,
                          also the on phase of a queued blink) or 0 (blink off phase).
                          lcd.c padled_set sends the bytes in GRB order. No gamma and
                          no current limit in the firmware.
  LCD (smk_clip_lcd_draw): current firmware: the brightest channel is mapped from 0..255
                          to SMK_CLIP_LCD_FLOOR..255 (hue kept), then RGB565.

Proposed LCD model (lcd_new, not in the firmware yet):
  1. The current floor gives (r, g, b) with brightest channel t.
  2. Target: relative luminance Y (gamma 2.2, Rec.709 weights) >= YMIN * (t/255)^2.2.
     A colour at or above the target does not change.
  3. Else, in OKLCH: keep the hue h of the palette colour constant, raise L until the
     target is reached. At each L, use the highest chroma C <= the start chroma that is
     in the sRGB gamut (binary search on C).
  4. RGB565: of the neighbour codes (each channel -1..+1 step around the rounded value)
     with Y >= the target, take the one with the smallest OKLCH hue error.
  5. Hue fix (HUE_FIX, only 0x32 and 0x33): the sRGB decode of a very dark palette value
     gives a hue bias. The 0.055 offset of the sRGB curve makes a weak channel (here red)
     too strong, so 0x33 0F0030 reads 10 deg more purple than its parent 0x31 5400FF.
     For these entries the target hue is the hue of the palette colour scaled to full
     brightness (brightest channel 255). This hue does not change with the scale.
     "strong" adds a further -5 deg OKLCH nudge toward blue. The user approved all
     other entries, so the fix applies to these two only (a global rule changes 7 more).
  The previous proposal (lcd_prev: blend toward grey(t) in sRGB) is kept for comparison.

Usage:
  tools/apc_pad_colors.py                 # table on stdout
  tools/apc_pad_colors.py --csv out.csv
  tools/apc_pad_colors.py --html out.html # swatch page
  tools/apc_pad_colors.py --c out.h       # APC40_LCD565[128] table for the firmware
  --hue-fix none|mild|strong              # hue fix for 0x32 and 0x33 (default strong, user choice)
"""
import argparse
import html
import math
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "firmware/src/app/main_fm1.c"
YMIN = 0.25         # luminance target for a full colour (t = 255)
# Hue fix: entries and the extra OKLCH hue nudge (degrees) for each strength.
HUE_FIX = (0x32, 0x33)
HUE_NUDGE = {"none": None, "mild": 0.0, "strong": -5.0}

# Velocities seen in Loopy Pro captures (APC40_IMPLEMENTATION.md 1.3-apc40, 1.5-apc40, follow-up 2).
SEEN = {
    0x0B: "idle / empty slot (dim shade)",
    0x1F: "filled clip, not playing (dim shade)",
    0x27: "filled clip, not playing (dim shade)",
    0x37: "filled clip, not playing (dim shade)",
    0x39: "playing (full shade, one capture)",
    0x60: "playing",
}
# Blue, violet and pink entries that Loopy can send (not captured).
CAND = {0x2D: "blue full", 0x29: "blue full", 0x25: "blue full", 0x2F: "blue idle",
        0x2B: "blue idle", 0x4F: "blue ext", 0x5C: "blue ext", 0x31: "violet full",
        0x33: "violet idle", 0x51: "violet ext", 0x50: "violet ext", 0x45: "violet ext",
        0x35: "magenta full", 0x5E: "pink ext", 0x5F: "pink ext", 0x52: "pink ext",
        0x3B: "pink idle"}


def palette():
    src = MAIN.read_text()
    m = re.search(r"APC40_RGB\[128\]\s*=\s*\{(.*?)\};", src, re.S)
    if not m:
        sys.exit("APC40_RGB[128] not found in main_fm1.c")
    pal = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{6})", m.group(1))]
    if len(pal) != 128:
        sys.exit(f"APC40_RGB has {len(pal)} entries, not 128")
    f = re.search(r"#define\s+SMK_CLIP_LCD_FLOOR\s+(\d+)u?", src)
    return pal, int(f.group(1)) if f else None


def split(rgb):
    return rgb >> 16, (rgb >> 8) & 0xFF, rgb & 0xFF


def pad_led(rgb, brightness=255):
    """Bytes that smk_pads gives to padled_set (R, G, B)."""
    return tuple((c * brightness) // 510 for c in split(rgb))


# --- colour science -------------------------------------------------------------
def lum(r, g, b):
    """Relative luminance with a 2.2 gamma (the target rule)."""
    return 0.2126 * (r / 255) ** 2.2 + 0.7152 * (g / 255) ** 2.2 + 0.0722 * (b / 255) ** 2.2


def _dec(c):            # sRGB transfer, 0..255 -> linear
    c /= 255
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def _enc(x):            # linear -> 0..255 (float, not clipped)
    v = 12.92 * x if x <= 0.0031308 else 1.055 * math.copysign(abs(x) ** (1 / 2.4), x) - 0.055
    return v * 255


def oklab(r, g, b):
    r, g, b = _dec(r), _dec(g), _dec(b)
    l = 0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b
    m = 0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b
    s = 0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b
    l, m, s = (math.copysign(abs(x) ** (1 / 3), x) for x in (l, m, s))
    return (0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
            1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
            0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s)


def oklab_rgb(L, a, b):
    l = (L + 0.3963377774 * a + 0.2158037573 * b) ** 3
    m = (L - 0.1055613458 * a - 0.0638541728 * b) ** 3
    s = (L - 0.0894841775 * a - 1.2914855480 * b) ** 3
    return (_enc(4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s),
            _enc(-1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s),
            _enc(-0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s))


def lch(r, g, b):
    L, a, bb = oklab(r, g, b)
    return L, math.hypot(a, bb), math.degrees(math.atan2(bb, a)) % 360


def hue_err(rgb, h0):
    """OKLCH hue difference in degrees (None for a near-grey colour)."""
    L, C, h = lch(*rgb)
    if C < 0.02:
        return None
    return (h - h0 + 180) % 360 - 180


def _lch_rgb(L, C, h):
    return oklab_rgb(L, C * math.cos(math.radians(h)), C * math.sin(math.radians(h)))


def _in_gamut(rgb):
    return all(-0.5 <= c <= 255.5 for c in rgb)


def _max_c(L, C0, h):
    """Highest chroma <= C0 at (L, h) that is in the sRGB gamut."""
    if _in_gamut(_lch_rgb(L, C0, h)):
        return C0
    lo, hi = 0.0, C0
    for _ in range(40):
        mid = (lo + hi) / 2
        if _in_gamut(_lch_rgb(L, mid, h)):
            lo = mid
        else:
            hi = mid
    return lo


def to565(r, g, b):
    return (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3)


def of565(c):
    """RGB565 back to 0..255 (float: the panel level of each code)."""
    return ((c >> 11) * 255 / 31, ((c >> 5) & 63) * 255 / 63, (c & 31) * 255 / 31)


def of565i(c):
    return tuple(int(round(x)) for x in of565(c))


# --- LCD models -------------------------------------------------------------------
def lcd_cur(rgb, floor):
    """Firmware 0fbfe07: brightest channel to floor..255, hue kept, then RGB565."""
    r, g, b = split(rgb)
    mx = max(r, g, b)
    if floor is not None and mx:
        t = floor + mx * (255 - floor) // 255
        r, g, b = r * t // mx, g * t // mx, b * t // mx
    return (r, g, b), to565(r, g, b)


def target(rgb8):
    t = max(rgb8)
    return YMIN * (t / 255) ** 2.2


def lcd_prev(rgb, floor):
    """First proposal: blend toward grey(t) in sRGB until the target. Shifts the hue."""
    (r, g, b), c = lcd_cur(rgb, floor)
    t = max(r, g, b)
    if not t or lum(r, g, b) >= target((r, g, b)):
        return (r, g, b), c
    yt = target((r, g, b))
    lo, hi = 0.0, 1.0
    for _ in range(30):
        s = (lo + hi) / 2
        if lum(*(x + s * (t - x) for x in (r, g, b))) >= yt:
            hi = s
        else:
            lo = s
    out = tuple(int(x + hi * (t - x) + .5) for x in (r, g, b))
    return out, to565(*out)


def lcd_new(rgb, floor, h0=None):
    """New proposal: raise OKLCH L at constant hue, chroma reduced only for the gamut.

    h0: target OKLCH hue. None: the hue of the palette colour (approved rule)."""
    (r, g, b), c = lcd_cur(rgb, floor)
    if not max(r, g, b):
        return (r, g, b), c
    yt = target((r, g, b))
    if lum(r, g, b) >= yt:
        return (r, g, b), c
    if h0 is None:
        h0 = lch(*split(rgb))[2]              # hue of the palette colour
    L0, C0, _ = lch(r, g, b)

    def at(L):
        return _lch_rgb(L, _max_c(L, C0, h0), h0)

    lo, hi = L0, 1.0
    for _ in range(40):
        mid = (lo + hi) / 2
        if lum(*(min(255.0, max(0.0, x)) for x in at(mid))) >= yt:
            hi = mid
        else:
            lo = mid
    ideal = tuple(min(255.0, max(0.0, x)) for x in at(hi))
    # RGB565: neighbours of the rounded code; keep Y >= target, smallest hue error,
    # then the smallest distance to the ideal colour.
    base = (round(ideal[0] * 31 / 255), round(ideal[1] * 63 / 255), round(ideal[2] * 31 / 255))
    best = None
    for dr in (-1, 0, 1):
        for dg in (-1, 0, 1):
            for db in (-1, 0, 1):
                q = (base[0] + dr, base[1] + dg, base[2] + db)
                if not (0 <= q[0] <= 31 and 0 <= q[1] <= 63 and 0 <= q[2] <= 31):
                    continue
                code = q[0] << 11 | q[1] << 5 | q[2]
                rgbq = of565(code)
                if lum(*rgbq) < yt * 0.995:
                    continue
                he = hue_err(rgbq, h0)
                key = (abs(he) if he is not None else 99,
                       sum((a - b_) ** 2 for a, b_ in zip(rgbq, ideal)))
                if best is None or key < best[0]:
                    best = (key, code)
    code = best[1]
    return tuple(int(round(x)) for x in ideal), code


def scaled_hue(rgb):
    """OKLCH hue of the palette colour scaled to full brightness (no dark-value bias)."""
    r, g, b = split(rgb)
    mx = max(r, g, b)
    return lch(*(x * 255 / mx for x in (r, g, b)))[2]


def fix_hue(v, rgb, strength):
    """Target hue for a HUE_FIX entry, or None (palette hue, approved rule)."""
    nudge = HUE_NUDGE[strength]
    if v not in HUE_FIX or nudge is None:
        return None
    return scaled_hue(rgb) + nudge


def rows(strength="strong"):
    pal, floor = palette()
    for v, rgb in enumerate(pal):
        led = pad_led(rgb)
        h0 = lch(*split(rgb))[2]
        cur8, cur565 = lcd_cur(rgb, floor)
        prev8, prev565 = lcd_prev(rgb, floor)
        base8, base565 = lcd_new(rgb, floor)            # approved new proposal
        alt = {k: lcd_new(rgb, floor, fix_hue(v, rgb, k))[1] for k in HUE_NUDGE if k != "none"}
        ht = fix_hue(v, rgb, strength)
        new8, new565 = lcd_new(rgb, floor, ht)
        yield dict(v=v, pal=rgb, led=led, grb=(led[1], led[0], led[2]), floor=floor,
                   cur8=cur8, cur565=cur565, prev8=prev8, prev565=prev565,
                   new8=new8, new565=new565, chg=bool(v) and new565 != cur565,
                   base565=base565, alt565=alt, fixed=v in HUE_FIX and ht is not None,
                   h_target=h0 if ht is None else ht, h_pal=h0, strength=strength,
                   he_cur=hue_err(of565(cur565), h0) if v else None,
                   he_prev=hue_err(of565(prev565), h0) if v else None,
                   he_base=hue_err(of565(base565), h0) if v else None,
                   he_new=hue_err(of565(new565), h0) if v else None,
                   lcd565=cur565, lcd=of565i(cur565),
                   x2=tuple(min(255, 2 * c) for c in led), seen=SEEN.get(v, ""))


def perceived(led, gamma=2.2):
    """Rough screen preview of a WS2812 level."""
    return tuple(round(255 * (c / 255) ** (1 / gamma)) for c in led)


def hx(t):
    return "#%02X%02X%02X" % tuple(int(round(x)) for x in t)


def fmt_he(x):
    return "-" if x is None else f"{x:+.2f}"


def c_table(data):
    out = ["/* Generated by tools/apc_pad_colors.py --c. LCD clip colour (RGB565) for each APC40",
           f" * velocity: SMK_CLIP_LCD_FLOOR {data[0]['floor']}, then OKLCH lift to Y >= {YMIN} * (t/255)^2.2",
           " * at constant hue. Index 0 is not drawn (empty cell).",
           f" * Hue fix ({data[0]['strength']}) for 0x32 and 0x33: target hue = hue of the palette colour",
           " * scaled to full brightness" + ("" if HUE_NUDGE[data[0]['strength']] in (None, 0.0)
                                             else f", {HUE_NUDGE[data[0]['strength']]:+.0f} deg") +
           ("." if HUE_NUDGE[data[0]['strength']] is not None else " (not applied).") + " */",
           "static const uint16_t APC40_LCD565[128] = {"]
    for i in range(0, 128, 8):
        out.append("    " + ",".join(f"0x{d['new565']:04X}" for d in data[i:i + 8]) + ",")
    out.append("};")
    return "\n".join(out) + "\n"


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--csv")
    ap.add_argument("--html")
    ap.add_argument("--c", dest="ctab")
    ap.add_argument("--hue-fix", choices=list(HUE_NUDGE), default="strong")
    a = ap.parse_args()
    data = list(rows(a.hue_fix))
    if a.csv:
        with open(a.csv, "w") as f:
            f.write("velocity,palette,led_r,led_g,led_b,blink_off,lcd_rgb565,lcd_rgb,"
                    "new_rgb565,new_rgb,new_hue_err,loopy_seen\n")
            for d in data:
                f.write(f"{d['v']},{d['pal']:06X},{d['led'][0]},{d['led'][1]},{d['led'][2]},0/0/0,"
                        f"{d['cur565']:04X},{hx(of565(d['cur565']))},{d['new565']:04X},"
                        f"{hx(of565(d['new565']))},{fmt_he(d['he_new'])},{d['seen']}\n")
    if a.html:
        Path(a.html).write_text(page(data))
    if a.ctab:
        Path(a.ctab).write_text(c_table(data))
    if not a.csv and not a.html and not a.ctab:
        print("vel  palette  pad LED R,G,B   LCD565 now  new    dh new  Loopy")
        for d in data:
            print(f"{d['v']:3d}  {d['pal']:06X}   {d['led'][0]:3d},{d['led'][1]:3d},{d['led'][2]:3d}"
                  f"    {d['cur565']:04X}      {d['new565']:04X}  {fmt_he(d['he_new']):>6}  {d['seen']}")


def hue_fix_section(data):
    """Side by side view of the hue fix for 0x32 and 0x33, with reference entries."""
    by = {d["v"]: d for d in data}
    refs = [0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33]

    def sw(d, c, show_fix):
        q = of565(c)
        L, C, h = lch(*q)
        tgt = target(d["cur8"])
        extra = (f' &middot; vs scaled {(h - scaled_hue(d["pal"]) + 180) % 360 - 180:+.1f}&deg;'
                 if show_fix else "")
        return (f'<td><span class="big"><span style="background:{hx(q)}"></span></span>{hx(q)}'
                f'<br><small>0x{c:04X} &middot; Y {lum(*q):.3f} (target {tgt:.3f})<br>'
                f'OKLCH {L:.3f} {C:.3f} {h:.1f}&deg; &middot; dh {(h - d["h_pal"] + 180) % 360 - 180:+.1f}&deg;'
                f'{extra}</small></td>')

    trs = []
    for v in refs:
        d = by[v]
        fx = v in HUE_FIX
        pL, pC, ph = lch(*split(d["pal"]))
        alt = [sw(d, d["alt565"][k], fx) if fx else '<td><small>no change</small></td>'
               for k in ("mild", "strong")]
        trs.append(f'<tr class="{"hl" if fx else ""}"><td>0x{v:02X}<br><small>{v}'
                   f'{"<br>" + html.escape(CAND.get(v, "")) if v in CAND else ""}</small></td>'
                   f'<td><span class="big"><span style="background:#{d["pal"]:06X}"></span></span>#{d["pal"]:06X}'
                   f'<br><small>OKLCH {pL:.3f} {pC:.3f} {ph:.1f}&deg;<br>scaled hue {scaled_hue(d["pal"]):.1f}&deg;</small></td>'
                   + sw(d, d["cur565"], fx) + sw(d, d["base565"], fx) + "".join(alt) + "</tr>")

    def strip(key):
        cells = []
        for v in refs:
            d = by[v]
            c = d["base565"] if key == "base" or v not in HUE_FIX else d["alt565"][key]
            cells.append(f'<span title="0x{v:02X}" style="background:{hx(of565(c))}"></span>')
        return f'<div class="grid">{"".join(cells)}</div>'

    return f"""<h2>Hue fix for 0x32 and 0x33: select a strength</h2>
<p>User feedback: 0x32 is slightly too purple and 0x33 is more purple than blue. Cause: the
approved rule takes the hue from the sRGB decode of the palette value. For dark values the
0.055 offset of the sRGB curve makes the weak red channel too strong. The hue of
0x33 0F0030 is 288.6&deg;, but the same colour at full brightness is 278.3&deg;, the hue of its
parent 0x31 5400FF (279.6&deg;). For 0x32 190064 the bias is 4&deg; (277.8&deg; to 273.8&deg;).
<b>Mild</b>: target hue = hue of the palette colour scaled to full brightness. <b>Strong</b>: mild
&minus;5&deg; OKLCH toward blue. All other entries do not change (0x2D to 0x31 are references).
The firmware table (apc40_lcd565.h) uses <b>{data[0]["strength"]}</b>.
dh = OKLCH hue difference to the palette hue. &ldquo;vs scaled&rdquo; = difference to the scaled hue.</p>
<table class="fixt"><tr><th>Velocity</th><th>Palette</th><th>Current LCD</th><th>Approved new (no fix)</th>
<th>Mild fix</th><th>Strong fix</th></tr>
{''.join(trs)}</table>
<p>LCD order preview, 0x2D 0x2E 0x2F 0x30 0x31 0x32 0x33:</p>
<p>Approved new: {strip("base")}<br><br>Mild fix: {strip("mild")}<br><br>Strong fix: {strip("strong")}</p>
"""


def page(data):
    floor = data[0]["floor"]
    changed = [d for d in data if d["chg"]]
    he = [abs(d["he_new"]) for d in changed if d["he_new"] is not None and not d["fixed"]]
    he_prev = [abs(d["he_prev"]) for d in data if d["v"] and d["prev565"] != d["cur565"] and d["he_prev"] is not None]

    def cell(c565, frame=False):
        return (f'<span class="cell"><span style="background:{hx(of565(c565))}"></span></span>')

    def col(d, k):
        if d["v"] == 0:
            return '<td><span class="dot"></span> empty: grey dot</td>'
        c = d[k + "565"]
        rgb = of565(c)
        mark = " &middot; <b>changed</b>" if k != "cur" and c != d["cur565"] else ""
        if k == "new" and d["fixed"]:
            mark += (f'<br><b class="fix">hue fix ({d["strength"]}): target h {d["h_target"]:.1f}&deg;, '
                     f'approved new was 0x{d["base565"]:04X} {hx(of565(d["base565"]))}</b>')
        return (f'<td>{cell(c)}{hx(rgb)}<br><small>565 0x{c:04X} &middot; Y {lum(*rgb):.3f} '
                f'&middot; dh {fmt_he(d["he_" + k])}&deg;{mark}</small></td>')

    fix_html = hue_fix_section(data)
    trs = []
    for d in data:
        cls = ("seen " if d["seen"] else "") + ("cand " if d["v"] in CAND else "") + ("chg" if d["chg"] else "")
        note = d["seen"] or (("candidate: " + CAND[d["v"]]) if d["v"] in CAND else "")
        trs.append(f'<tr class="{cls}"><td>{d["v"]}<br><small>0x{d["v"]:02X}</small></td>'
                   f'<td><span class="cell"><span style="background:#{d["pal"]:06X}"></span></span>#{d["pal"]:06X}'
                   f'<br><small>Y {lum(*split(d["pal"])):.3f} &middot; h {lch(*split(d["pal"]))[2]:.1f}&deg;</small></td>'
                   + col(d, "cur") + col(d, "prev") + col(d, "new") +
                   f'<td>{html.escape(note)}</td></tr>')
    loopy = [("orange border", "#E98F35"), ("orange playing fill", "#8F5D22"), ("yellow", "#CCBA43"),
             ("green", "#AADB49"), ("pink", "#E787BC")]
    return f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><title>SMK-37 LCD clip colours: current and proposals</title>
<style>
body {{ font: 13px -apple-system, sans-serif; background: #111; color: #ddd; margin: 20px; }}
table {{ border-collapse: collapse; }} td, th {{ border-bottom: 1px solid #333; padding: 4px 8px; text-align: left; vertical-align: middle; }}
th {{ position: sticky; top: 0; background: #222; }}
.cell {{ display: inline-block; width: 30px; height: 30px; background: #000; padding: 3px; margin: 3px 8px 3px 3px; vertical-align: middle; box-sizing: border-box; }}
.cell span {{ display: block; width: 100%; height: 100%; }}
.dot {{ display: inline-block; width: 6px; height: 6px; background: rgb(74,74,82); margin: 0 20px; }}
tr.seen td {{ background: #2a2a10; }} tr.cand td:first-child {{ border-left: 4px solid #6a6aff; }}
tr.chg td:nth-child(5) {{ background: #1d2a1d; }}
small {{ color: #999; }} .fix {{ color: #ffb347; }}
.big {{ display: inline-block; width: 64px; height: 44px; background: #000; padding: 4px; box-sizing: border-box; vertical-align: middle; margin-right: 8px; }}
.big span {{ display: block; width: 100%; height: 100%; }}
.grid {{ display: inline-flex; gap: 3px; background: #000; padding: 6px; }} .grid span {{ display: block; width: 40px; height: 28px; }}
table.fixt td {{ border-bottom: 1px solid #444; }} tr.hl td {{ background: #2a1d2a; }} p, ul {{ max-width: 1000px; }}
.sw {{ display: inline-block; width: 40px; height: 20px; vertical-align: middle; margin-right: 6px; }}
</style></head><body>
<h1>SMK-37 clip mode: LCD clip colours</h1>
<p>Generated by tools/apc_pad_colors.py --html from <code>APC40_RGB[128]</code> in firmware/src/app/main_fm1.c.
Cells are drawn on black as on the LCD. Each colour is the RGB565 code decoded back to 8 bits.
dh = OKLCH hue error of the LCD colour against the palette colour (degrees).</p>
<ul>
<li><b>Current LCD</b> (firmware 0fbfe07): brightest channel mapped 0..255 &rarr; {floor}..255, then RGB565.</li>
<li><b>Previous proposal</b>: current, then blend toward grey(t) in sRGB until Y &ge; {YMIN} &times; (t/255)<sup>2.2</sup>. Max |dh| of changed entries: {max(he_prev):.2f}&deg;.</li>
<li><b>New proposal</b>: current, then raise OKLCH L at the palette hue until the same target; chroma is reduced only to stay in the sRGB gamut; RGB565 neighbour with the smallest hue error. {len(changed)} of 127 lit entries change (green background). Max |dh| of changed entries: <b>{max(he):.2f}&deg;</b> (without 0x32 and 0x33, see the hue fix).</li>
<li>Yellow rows: velocities seen in Loopy Pro captures. Blue left edge: blue, violet and pink entries that Loopy can send (not captured).</li>
<li>Empty-cell grey dot (74,74,82): Y 0.067.</li>
</ul>
<p>Loopy Pro UI colours (sampled from caps/l1.png): {''.join(f'<span class="sw" style="background:{c}"></span>{n} {c} &nbsp; ' for n, c in loopy)}</p>
{fix_html}
<h2>All entries</h2>
<table><tr><th>Velocity</th><th>Palette (Akai)</th><th>Current LCD</th><th>Previous proposal (sRGB grey blend)</th>
<th>New proposal (OKLCH, constant hue, hue fix {data[0]['strength']})</th><th>Loopy</th></tr>
{''.join(trs)}
</table></body></html>
"""


if __name__ == "__main__":
    main()
