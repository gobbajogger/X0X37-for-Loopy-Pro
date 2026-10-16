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
# LCD (RGB565, the ST7789V panel has its own gamma): no gamma. The brightest channel maps
# from 0..255 to LCD_FLOOR..255 (idle shades are too dark on the LCD). If the colour is darker
# than Y >= LCD_YMIN * (t/255)^2.2, OKLCH L goes up at the hue of the source colour
# (owner rule: keep the hue, blue must not go toward purple).
LCD_FLOOR = 128
LCD_YMIN = 0.25
# The owner approved the "strong" hue fix for 0x32 and 0x33 (USER_PREFERENCES, LCD clip colours):
# for these the target hue is the hue at full brightness, plus LCD_HUE_NUDGE degrees (toward blue).
LCD_HUE_FIX = (0x32, 0x33)
LCD_HUE_NUDGE = -5.0


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
    return [x * k for x in d]


def pad_level(e, mode=None, gamma=None, balance=None):
    """Pad table entry (R, G, B 0..255) for one velocity."""
    rgb = target(e, mode)
    if not rgb:
        return (0, 0, 0)
    d = pad_linear(rgb, gamma, balance)
    mx = max(d)
    if mx <= 0:
        return (0, 0, 0)
    floor = (2 * PAD_FLOOR_SENT) / 255          # table units: send = level * 255 // 510
    if mx < floor:
        d = [x * floor / mx for x in d]
    return tuple(min(255, int(round(255 * x))) for x in d)


def pad_send(levels, bright=255):
    return tuple(c * bright // 510 for c in levels)


# --- LCD ------------------------------------------------------------------------
def to565(r, g, b):
    return (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3)


def of565(c):
    return ((c >> 11) * 255 / 31, ((c >> 5) & 63) * 255 / 63, (c & 31) * 255 / 31)


def hue_err(rgb, h0):
    L, C, h = lch(*rgb)
    if C < 0.02:
        return None
    return (h - h0 + 180) % 360 - 180


def _lch_rgb(L, C, h):
    return oklab_rgb(L, C * math.cos(math.radians(h)), C * math.sin(math.radians(h)))


def _in_gamut(rgb):
    return all(-0.5 <= c <= 255.5 for c in rgb)


def _max_c(L, C0, h):
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


def lcd_floor(rgb):
    r, g, b = split(rgb)
    mx = max(r, g, b)
    if mx:
        t = LCD_FLOOR + mx * (255 - LCD_FLOOR) // 255
        r, g, b = r * t // mx, g * t // mx, b * t // mx
    return r, g, b


def lcd565(e, mode=None):
    """LCD table entry (RGB565): the owner-approved rule (USER_PREFERENCES, LCD clip colours).

    Floor (LCD_FLOOR), then the OKLCH lift at the hue of the source colour. For the
    LCD_HUE_FIX velocities the target hue is the hue at full brightness plus LCD_HUE_NUDGE."""
    rgb = target(e, mode)
    if not rgb:
        return 0
    r, g, b = lcd_floor(rgb)
    yt = LCD_YMIN * (max(r, g, b) / 255) ** 2.2
    if lum(r, g, b) >= yt:
        return to565(r, g, b)
    h0 = full_hue(rgb) + LCD_HUE_NUDGE if e["v"] in LCD_HUE_FIX else lch(*split(rgb))[2]
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
                key = (abs(he) if he is not None else 99, sum((a - b_) ** 2 for a, b_ in zip(rgbq, ideal)))
                if best is None or key < best[0]:
                    best = (key, code)
    return best[1]
