"""Render a 303 note crossing a page and a pattern loop, then check edge cues."""

import pathlib
import subprocess
import sys

from PIL import Image


root = pathlib.Path(__file__).resolve().parents[2]
binary = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else root / "build/host/x0x_host")
out = pathlib.Path(sys.argv[2] if len(sys.argv) > 2 else root / "build/roll-continuation")
out.mkdir(parents=True, exist_ok=True)
subprocess.run([str(binary), str(root / "tests/scenarios/roll_continuation.x0x"), str(out)], check=True)


def green(name, x, y):
    rgb = Image.open(out / name).convert("RGB").getpixel((x, y))
    return rgb[1] > rgb[0] + 30 and rgb[1] > rgb[2] + 30


for stem in ("page", "loop"):
    assert not green(f"roll_{stem}_out.png", 237, 102), f"{stem}: premature right cue"
    assert green(f"roll_{stem}_out_connected.png", 237, 102), f"{stem}: missing right cue"
    assert green(f"roll_{stem}_in.png", 21, 102), f"{stem}: missing left cue"

short = out / "short"
short.mkdir(exist_ok=True)
subprocess.run([str(binary), str(root / "tests/scenarios/roll_short_tie.x0x"), str(short)], check=True)
short_image = Image.open(short / "fractional_tie.png").convert("RGB")
assert short_image.getpixel((53, 102))[1] > 70, "short tied endpoint has no brick"
assert short_image.getpixel((56, 102))[1] < 70, "connector overdraws short tied endpoint"

print("roll continuation display: page and loop cues PASS")
