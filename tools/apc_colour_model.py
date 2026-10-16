# SPDX-License-Identifier: GPL-3.0-only
"""Colour model for tools/apc_pad_colors.py: one source, pad LED levels and LCD RGB565.

Source: tools/apc40mk2_akai_protocol_v1.2.csv, a copy of the "RGB LEDs" table of the
"Akai APC40 Mk2 Communications Protocol" v1.2 (January 19, 2015), pages 17-21:
https://cdn.inmusicbrands.com/akai/attachments/apc40II/APC40Mk2_Communications_Protocol_v1.2.pdf
The APC mini mk2 protocol v1.0 and two public copies give the same 128 values.
No colour value is picked by hand. Each step is a rule with named constants.
"""
import csv
import math
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "tools/apc40mk2_akai_protocol_v1.2.csv"

# Step 1: the colour for a velocity. "apc": the Color column (the value that an APC40 Mk2 shows).
# "live": for 60-119 the Name column (the Ableton Live clip colour), else the Color column.
TARGET = "apc"
# Step 2 (pads): the source value is sRGB-like. A WS2812 LED is linear in its PWM level, so the
# tool decodes the value with a power curve. Advice for WS2812: 2.2 to 2.8 (Adafruit, "LED tricks:
# gamma correction"). The owner selects the value by eye (PAD_GAMMA_CHOICES).
PAD_GAMMA = 2.2
PAD_GAMMA_CHOICES = (1.0, 2.2, 2.8)
# Step 3 (pads): channel balance from the WS2812B data sheet (Worldsemi WS2812B, table "RGB IC
# characteristic parameter": red 620-630 nm 550-700 mcd, green 515-530 nm 1100-1400 mcd, blue
# 465-475 nm 200-400 mcd; the middle of each range). The tool solves the LED levels that give the
# white gains: the LED levels that give the D65 white point (PAD_WHITE_GAIN, about 1, 1, 0.49).
PAD_BALANCE = True
LED_NM = (625.0, 522.5, 470.0)
LED_MCD = (625.0, 1250.0, 300.0)
# CIE 1931 2 degree colour matching functions (x, y, z) at LED_NM (5 nm table; 522.5 nm is the
# mean of 520 and 525 nm).
CIE_CMF = ((0.7514, 0.3210, 0.0001),
           ((0.06327 + 0.1096) / 2, (0.7100 + 0.7932) / 2, (0.07825 + 0.05725) / 2),
           (0.19536, 0.09098, 1.28764))
# Step 4 (pads): the firmware sends level * brightness / 510 (main_fm1.c smk_pads, half level).
# A lit entry sends at least PAD_FLOOR_SENT on its brightest channel, so the dim 0x19 entries
# still show. The floor scales all three channels: the colour ratio stays.
PAD_FLOOR_SENT = 2
# Step 5 (pads): a trim of each channel (R, G, B) after the white gains, for the owner's eye.
PAD_GAIN_TRIM = (1.0, 1.0, 1.0)
# LCD (RGB565, the ST7789V panel has its own gamma): the source colour as plain RGB565, no gamma,
# no floor, no lift (Orchestrator decision 2026-10-16 23:10: no hand-picked values). Each channel
# goes to the nearest 5, 6, 5 bit level (rounding: the dark violets keep their hue better than with
# the high bits only; tests/apc_palette_test.py).


def load_source(path=SRC):
    rows = list(csv.DictReader(path.read_text().splitlines()))
    assert [int(r["index"]) for r in rows] == list(range(128)), "source must have index 0-127"
    out = []
    for r in rows:
        name = r["name_col"]
        live = int(name[2:], 16) if name.startswith("0x") else None
        out.append(dict(v=int(r["index"]), apc=int(r["hex"][1:], 16), live=live, name=name))
    return out


def split(rgb):
    return rgb >> 16, (rgb >> 8) & 0xFF, rgb & 0xFF


def join(r, g, b):
    return r << 16 | g << 8 | b


def target(e, mode=None):
    mode = mode or TARGET
    return e["live"] if mode == "live" and e["live"] is not None else e["apc"]


# --- colour science -------------------------------------------------------------
def _dec(c):
    c /= 255
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def _enc(x):
    v = 12.92 * x if x <= 0.0031308 else 1.055 * math.copysign(abs(x) ** (1 / 2.4), x) - 0.055
    return v * 255


def lum(r, g, b):
    return 0.2126 * (r / 255) ** 2.2 + 0.7152 * (g / 255) ** 2.2 + 0.0722 * (b / 255) ** 2.2


SRGB_XYZ = ((0.4124564, 0.3575761, 0.1804375), (0.2126729, 0.7151522, 0.0721750),
            (0.0193339, 0.1191920, 0.9503041))


def lin_xyz(r, g, b):
    return tuple(m[0] * r + m[1] * g + m[2] * b for m in SRGB_XYZ)


def xyz_oklab(X, Y, Z):
    l = 0.8189330101 * X + 0.3618667424 * Y - 0.1288597137 * Z
    m = 0.0329845436 * X + 0.9293118715 * Y + 0.0361456387 * Z
    s = 0.0482003018 * X + 0.2643662691 * Y + 0.6338517070 * Z
    l, m, s = (math.copysign(abs(x) ** (1 / 3), x) for x in (l, m, s))
    return (0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
            1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
            0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s)


def oklab(r, g, b):
    return xyz_oklab(*lin_xyz(_dec(r), _dec(g), _dec(b)))


def oklab_rgb(L, a, b):
    l = (L + 0.3963377774 * a + 0.2158037573 * b) ** 3
    m = (L - 0.1055613458 * a - 0.0638541728 * b) ** 3
    s = (L - 0.0894841775 * a - 1.2914855480 * b) ** 3
    return (_enc(4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s),
            _enc(-1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s),
            _enc(-0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s))


def lch_of_lab(L, a, b):
    return L, math.hypot(a, b), math.degrees(math.atan2(b, a)) % 360


def lch(r, g, b):
    return lch_of_lab(*oklab(r, g, b))


def full(rgb):
    r, g, b = split(rgb)
    mx = max(r, g, b)
    return (0.0, 0.0, 0.0) if not mx else tuple(x * 255 / mx for x in (r, g, b))


def full_hue(rgb):
    """OKLCH hue of the colour scaled to brightest channel 255 (no dark-value bias)."""
    return lch(*full(rgb))[2]


def full_chroma(rgb):
    return lch(*full(rgb))[1]


# --- pads -----------------------------------------------------------------------
def _led_primaries():
    return [(x / y * mcd, mcd, z / y * mcd) for (x, y, z), mcd in zip(CIE_CMF, LED_MCD)]


LED_XYZ = _led_primaries()


def _det(a):
    return (a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0])
            + a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]))


def _solve_led(xyz):
    a = [[LED_XYZ[c][r] for c in range(3)] for r in range(3)]
    d = _det(a)
    out = []
    for c in range(3):
        b = [row[:] for row in a]
        for r in range(3):
            b[r][c] = xyz[r]
        out.append(_det(b) / d)
    return out


_WHITE = _solve_led(lin_xyz(1.0, 1.0, 1.0))
PAD_WHITE_GAIN = tuple(x / max(_WHITE) for x in _WHITE)   # the LED levels for D65 white


def led_xyz(levels):
    return tuple(sum(LED_XYZ[c][k] * levels[c] / 255 for c in range(3)) for k in range(3))


def led_hue(levels):
    """OKLCH hue and chroma of the light of the LED levels (chromaticity only)."""
    X, Y, Z = led_xyz(levels)
    if Y <= 0:
        return None, 0.0
    return lch_of_lab(*xyz_oklab(X / Y, 1.0, Z / Y))[2:0:-1]


def pad_linear(rgb, gamma=None, balance=None):
    """Linear LED drive (0..1 each) for a source colour, before the floor and rounding.

    Decode with the power curve (step 2). Multiply by the white gains (step 3): the LED levels
    that give D65 white, scaled to brightest 1. Then scale the colour back so that its
    brightest channel is the brightest decoded channel. A pure primary stays a pure primary
    at full level, and the mixes get the channel balance of the LED."""
    gamma = PAD_GAMMA if gamma is None else gamma
    balance = PAD_BALANCE if balance is None else balance
    lin = [(c / 255) ** gamma for c in split(rgb)]
    if not balance or not max(lin):
        return lin
    d = [x * w for x, w in zip(lin, PAD_WHITE_GAIN)]
    k = max(lin) / max(d)
    return [x * k * t for x, t in zip(d, PAD_GAIN_TRIM)]


# The firmware makes the start values at run time (app/smk_padcal.h smk_padcal_default) from the
# LCD table (the source colour as RGB565), so the 384 B table is not in flash. These integer steps
# are the same in the firmware: green uses its 5 high bits, a 32-entry gamma table, Q8 gains.
def pad_gamma_lut(gamma=None):
    gamma = PAD_GAMMA if gamma is None else gamma
    return [int(round(255 * (i / 31) ** gamma)) for i in range(32)]


def pad_gain_q8(balance=None):
    balance = PAD_BALANCE if balance is None else balance
    g = PAD_WHITE_GAIN if balance else (1.0, 1.0, 1.0)
    return [int(round(256 * w * t)) for w, t in zip(g, PAD_GAIN_TRIM)]


PAD_FLOOR_LEVEL = 2 * PAD_FLOOR_SENT          # table units: send = level * 255 // 510


def pad_default(c565, lut, gain, floor=PAD_FLOOR_LEVEL):
    """Start values (R, G, B 0..255) of one entry from its RGB565 code: decode with the gamma
    table, the white gains, then scale back to the brightest decoded channel. A lit entry gets
    at least floor on its brightest channel: below it, the 5-bit codes scaled to floor."""
    q = (c565 >> 11, (c565 >> 6) & 31, c565 & 31)
    if not c565:
        return (0, 0, 0)
    lin = [lut[x] for x in q]
    d = [x * g for x, g in zip(lin, gain)]
    lvl = [x * max(lin) // max(d) for x in d] if max(d) else [0, 0, 0]
    if max(lvl) < floor:                      # dim: the RGB565 code gives the hue (the gamma table has too few steps)
        lvl = [x * floor // max(q) for x in q]
    return tuple(min(255, x) for x in lvl)


def pad_level(e, mode=None, gamma=None, balance=None):
    """Pad start values (R, G, B 0..255) for one velocity: pad_default of its RGB565 code."""
    return pad_default(to565(*split(target(e, mode))), pad_gamma_lut(gamma), pad_gain_q8(balance))


def read_override(path):
    """The pad calibration export (tools/ble_autotest.py --padcal-export): lines
    "padcal VELOCITY R G B ok" or "... -". Return {velocity: (R, G, B)} of the entries marked ok."""
    out = {}
    for line in Path(path).read_text().splitlines():
        f = line.split()
        if len(f) == 2 and f[0] == "padcal_orient" and f[1] != "0":
            # CLP-79: the owner found another pad top-left. The entries are colours by velocity, so the
            # table does not change, but the firmware's pad order (SMK_APC40 mapping) is then wrong.
            print(f"warning: {path}: padcal_orient {f[1]}: the pad order of the firmware is not as assumed "
                  "(BEHAVIOR.md CLP-79)", file=sys.stderr)
        if len(f) == 6 and f[0] == "padcal" and f[5] == "ok":
            v, r, g, b = (int(x) for x in f[1:5])
            assert 0 <= v < 128 and all(0 <= c <= 255 for c in (r, g, b)), line
            out[v] = (r, g, b)
    return out


def pad_send(levels, bright=255):
    return tuple(c * bright // 510 for c in levels)


# --- LCD ------------------------------------------------------------------------
def to565(r, g, b):
    """Nearest RGB565 code."""
    return (r * 31 + 127) // 255 << 11 | (g * 63 + 127) // 255 << 5 | (b * 31 + 127) // 255


# CLP-05 idle squares: Loopy draws a clip that does not play (and an empty slot, LIM-03) in the darkest shade
# of an Akai group of 4 (velocity v, v % 4 == 3). The groups are 4-59. The full shade of the group is v - 2.
IDLE_SHADES = tuple(range(7, 60, 4))


def lcd_idle565(src, v, mode=None):
    """LCD colour of the idle velocity v: half of the full shade v - 2 (8-bit, round half up), as RGB565."""
    assert v in IDLE_SHADES, v
    return to565(*((c + 1) // 2 for c in split(target(src[v - 2], mode))))


# CLP-79 entry names (dev build): a short hue family and shade from the Akai reference colour. They are
# descriptive names made here, not Akai names (the CSV has none) and not Loopy Pro names.
NAME_HUES = ("BLACK", "GREY", "WHITE", "RED", "ORANGE", "YELLOW", "LIME", "GREEN", "MINT", "CYAN", "SKY", "BLUE",
             "VIOLET", "MAGENTA", "ROSE")
NAME_SHADES = ("LIGHT", "FULL", "DIM", "DARK", "")
_HUE_LIMITS = ((15, "RED"), (40, "ORANGE"), (65, "YELLOW"), (95, "LIME"), (145, "GREEN"), (170, "MINT"), (195, "CYAN"),
               (220, "SKY"), (250, "BLUE"), (280, "VIOLET"), (320, "MAGENTA"), (345, "ROSE"), (361, "RED"))


def entry_name(src, v):
    """(hue index, shade index) of velocity v. Groups 4-59 (4 shades: light, full, dim, dark) take the hue
    of the full shade of the group and the shade from the position. Other entries: the shade from the
    brightest channel (FULL from 0xA0, DIM from 0x40, else DARK; LIGHT for a bright pale colour)."""
    import colorsys
    def hue_of(rgb):
        r, g, b = split(rgb)
        h, s, val = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
        mx = max(r, g, b)
        if mx == 0:
            return "BLACK"
        if s < 0.2:
            return "WHITE" if mx >= 0xE0 else "GREY"
        return next(n for t, n in _HUE_LIMITS if h * 360 < t)
    rgb = src[v]["apc"]
    hue = hue_of(src[v - (v - 4) % 4 + 1]["apc"] if 4 <= v < 60 else rgb)
    if hue in ("BLACK", "WHITE"):
        shade = 4
    elif 4 <= v < 60:
        shade = (v - 4) % 4
    else:
        r, g, b = split(rgb)
        mx = max(r, g, b)
        sat = (mx - min(r, g, b)) / mx
        shade = 0 if hue != "GREY" and mx >= 0xC0 and sat < 0.6 else 1 if mx >= 0xA0 else 2 if mx >= 0x40 else 3
    return NAME_HUES.index(hue), shade


def entry_name_text(src, v):
    h, s = entry_name(src, v)
    return (NAME_HUES[h] + " " + NAME_SHADES[s]).strip()


def of565(c):
    return ((c >> 11) * 255 / 31, ((c >> 5) & 63) * 255 / 63, (c & 31) * 255 / 31)


def lcd565(e, mode=None):
    """LCD table entry (RGB565): the source colour as plain RGB565."""
    return to565(*split(target(e, mode)))
