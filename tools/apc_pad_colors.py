#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""SMK-37 clip mode: calculate the pad LED and LCD colour for each APC40 mkII velocity.

The script reads APC40_RGB[128] from firmware/src/app/main_fm1.c and applies the
same integer arithmetic as the firmware:

  pads (smk_pads):        led = palette * brightness / 510, brightness 255 (steady,
                          also the on phase of a queued blink) or 0 (blink off phase).
                          lcd.c padled_set sends the bytes in GRB order. No gamma and
                          no current limit in the firmware.
  LCD (smk_clip_lcd_draw): the brightest channel is mapped from 0..255 to
                          SMK_CLIP_LCD_FLOOR..255 (hue kept), then RGB565.

Usage:
  tools/apc_pad_colors.py                 # table on stdout
  tools/apc_pad_colors.py --csv out.csv
  tools/apc_pad_colors.py --html out.html # swatch page
"""
import argparse
import html
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "firmware/src/app/main_fm1.c"

# Velocities seen in Loopy Pro captures (APC40_IMPLEMENTATION.md 1.3-apc40, 1.5-apc40, follow-up 2).
SEEN = {
    0x0B: "idle / empty slot (dim shade)",
    0x1F: "filled clip, not playing (dim shade)",
    0x27: "filled clip, not playing (dim shade)",
    0x37: "filled clip, not playing (dim shade)",
    0x39: "playing (full shade, one capture)",
    0x60: "playing",
}


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


def lcd(rgb, floor):
    """Colour that smk_clip_lcd_draw puts on the LCD, as RGB565 and as 8-bit RGB."""
    r, g, b = split(rgb)
    mx = max(r, g, b)
    if floor is not None and mx:
        t = floor + mx * (255 - floor) // 255
        r, g, b = r * t // mx, g * t // mx, b * t // mx
    c565 = (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3)
    return c565, ((c565 >> 11) * 255 // 31, ((c565 >> 5) & 63) * 255 // 63, (c565 & 31) * 255 // 31)


def perceived(led, gamma=2.2):
    """Rough screen preview of a WS2812 level. The LED light is linear in the PWM value.
    A screen is gamma encoded, so encode the linear level with 1/gamma."""
    return tuple(round(255 * (c / 255) ** (1 / gamma)) for c in led)


def hx(t):
    return "#%02X%02X%02X" % t


def rows():
    pal, floor = palette()
    for v, rgb in enumerate(pal):
        led = pad_led(rgb)
        c565, lcd8 = lcd(rgb, floor)
        yield dict(v=v, pal=rgb, led=led, grb=(led[1], led[0], led[2]), off=(0, 0, 0),
                   lcd565=c565, lcd=lcd8, x2=tuple(min(255, 2 * c) for c in led),
                   seen=SEEN.get(v, ""))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--csv")
    ap.add_argument("--html")
    a = ap.parse_args()
    data = list(rows())
    if a.csv:
        with open(a.csv, "w") as f:
            f.write("velocity,palette,led_r,led_g,led_b,blink_off,lcd_rgb565,lcd_rgb,loopy_seen\n")
            for d in data:
                f.write(f"{d['v']},{d['pal']:06X},{d['led'][0]},{d['led'][1]},{d['led'][2]},0/0/0,"
                        f"{d['lcd565']:04X},{hx(d['lcd'])},{d['seen']}\n")
    if a.html:
        Path(a.html).write_text(page(data))
    if not a.csv and not a.html:
        print("vel  palette  pad LED R,G,B (steady)  blink-off  LCD565  LCD RGB   Loopy")
        for d in data:
            print(f"{d['v']:3d}  {d['pal']:06X}   {d['led'][0]:3d},{d['led'][1]:3d},{d['led'][2]:3d}"
                  f"          0,0,0      {d['lcd565']:04X}    {hx(d['lcd'])}  {d['seen']}")


def page(data):
    _, floor = palette()
    cells = []
    for d in data:
        cls = "seen" if d["seen"] else ""
        cells.append(
            f'<tr class="{cls}"><td>{d["v"]}<br><small>0x{d["v"]:02X}</small></td>'
            f'<td><span class="sw" style="background:#{d["pal"]:06X}"></span>#{d["pal"]:06X}</td>'
            f'<td><span class="sw" style="background:{hx(d["led"])}"></span>{d["led"][0]}, {d["led"][1]}, {d["led"][2]}'
            f'<br><small>GRB bytes {d["grb"][0]}, {d["grb"][1]}, {d["grb"][2]}</small></td>'
            f'<td><span class="sw" style="background:{hx(d["x2"])}"></span>{hx(d["x2"])}</td>'
            f'<td><span class="sw" style="background:{hx(perceived(d["led"]))}"></span>{hx(perceived(d["led"]))}</td>'
            f'<td><span class="sw" style="background:#000"></span>0, 0, 0</td>'
            f'<td><span class="sw" style="background:{hx(d["lcd"])}"></span>{hx(d["lcd"])}<br><small>RGB565 0x{d["lcd565"]:04X}</small></td>'
            f'<td>{html.escape(d["seen"])}</td></tr>')
    return f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><title>SMK-37 clip mode: APC40 velocity colours</title>
<style>
body {{ font: 13px -apple-system, sans-serif; background: #111; color: #ddd; margin: 20px; }}
table {{ border-collapse: collapse; }} td, th {{ border-bottom: 1px solid #333; padding: 4px 8px; text-align: left; vertical-align: middle; }}
th {{ position: sticky; top: 0; background: #222; }}
.sw {{ display: inline-block; width: 34px; height: 22px; border: 1px solid #555; margin-right: 6px; vertical-align: middle; }}
tr.seen td {{ background: #2a2a10; }} small {{ color: #999; }} p {{ max-width: 900px; }}
</style></head><body>
<h1>SMK-37 clip mode: APC40 mkII velocity &rarr; pad and LCD colour</h1>
<p>Generated by tools/apc_pad_colors.py from <code>APC40_RGB[128]</code> in firmware/src/app/main_fm1.c.
The table is identical to the "RGB LEDs" table of the Akai APC40 Mk2 Communication Protocol v1.2.
Yellow rows: velocities seen in Loopy Pro captures.</p>
<p><b>Pad LED (steady)</b>: the exact bytes the firmware sends: palette &times; 255 / 510 (half level).
The on phase of a queued blink is the same. The off phase is 0, 0, 0. The WS2812 chain gets them in GRB order.
The firmware applies no gamma and no current limit.<br>
<b>Pad &times;2</b>: the LED value doubled, only to make it visible on a screen.<br>
<b>Pad perceived (approx.)</b>: the LED value with a simple 1/2.2 gamma. A WS2812 is linear in light, a screen is not,
so this is closer to what the eye sees. A real LED behind a pad cap still looks different from a screen
(brighter, more saturated, the diffuser changes it).<br>
<b>LCD</b>: the clip map colour on the screen (brightest channel mapped to {floor}..255, then RGB565).</p>
<table><tr><th>Velocity</th><th>Palette (Akai)</th><th>Pad LED steady (R,G,B)</th><th>Pad &times;2 (view)</th>
<th>Pad perceived (approx.)</th><th>Blink off</th><th>LCD clip map</th><th>Loopy</th></tr>
{''.join(cells)}
</table></body></html>
"""


if __name__ == "__main__":
    main()
