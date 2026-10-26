"""Exercise the actual 240px EDIT renderer and its transient envelope display."""

import pathlib
import subprocess
import sys

from PIL import Image, ImageChops


root = pathlib.Path(__file__).resolve().parents[2]
binary = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else root / "build/host/x0x_host_matrix_tagged"
out = pathlib.Path(sys.argv[2]) if len(sys.argv) > 2 else root / "build/ui-matrix-proof"

for engine in ("miami", "chicago", "drum"):
    dest = out / engine
    dest.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [str(binary), str(root / f"tests/scenarios/sound_matrix_{engine}.x0x"), str(dest)],
        check=True,
    )

for scenario, folder in (("sound_matrix_chicago_idle", "chicago-clean"),
                         ("sound_matrix_lock", "locked")):
    dest = out / folder
    dest.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(binary), str(root / f"tests/scenarios/{scenario}.x0x"), str(dest)], check=True)


def image(group, name):
    im = Image.open(out / group / name).convert("RGB")
    assert im.size == (240, 240), (group, name, im.size)
    return im


def different(group, a, b, area):
    assert ImageChops.difference(image(group, a).crop(area), image(group, b).crop(area)).getbbox(), (
        group, a, b, area
    )


def identical(group, a, b, area):
    assert not ImageChops.difference(image(group, a).crop(area), image(group, b).crop(area)).getbbox(), (
        group, a, b, area
    )


# One matrix of four current controls occupies the EDIT main area.  The old
# physical-knob strip is cleared, including while a control is touched.
for group, name in (
    ("miami", "miami_sound.png"),
    ("chicago", "chicago_sound.png"),
    ("drum", "drum_sd_edit.png"),
    ("drum", "drum_sd_touch.png"),
):
    im = image(group, name)
    assert not im.crop((0, 158, 240, 240)).getbbox(), (group, name, "duplicate knob row")
    assert im.crop((5, 40, 233, 153)).getbbox(), (group, name, "empty matrix")

# Chicago's continuous 0..127 saw-to-square control must visibly morph at 64.
chicago_wave = (158, 57, 204, 81)
different("chicago", "chicago_saw.png", "chicago_mix.png", chicago_wave)
different("chicago", "chicago_mix.png", "chicago_square.png", chicago_wave)
different("chicago", "chicago_saw.png", "chicago_square.png", chicago_wave)

# Miami's three wave controls are actual two-state choices.  X-mod's second
# state is the engine's 10% pulse; the carrier and wobble choose triangle/square.
different("miami", "miami_triangle.png", "miami_square.png", (38, 57, 82, 81))
different("miami", "miami_xmod_triangle.png", "miami_xmod_pulse.png", (158, 57, 204, 81))
different("miami", "miami_wobble_triangle.png", "miami_wobble_square.png", (38, 57, 82, 81))

# A knob edit opens the full envelope; it stays during the 500ms inactivity
# interval, follows later sustain edits, and closes after the 900ms grace.
graph = (15, 47, 211, 84)
different("miami", "miami_adsr_idle.png", "miami_adsr_active.png", graph)
identical("miami", "miami_adsr_active.png", "miami_adsr_grace.png", graph)
different("miami", "miami_adsr_active.png", "miami_adsr_sustain.png", graph)
different("miami", "miami_adsr_sustain.png", "miami_adsr_done.png", graph)
different("chicago", "chicago_adsr_idle.png", "chicago_adsr_active.png", graph)
different("chicago", "chicago_adsr_active.png", "chicago_adsr_done.png", graph)

# A SOUND-page detent must not activate the envelope after rapid page turns.
assert not ImageChops.difference(
    image("chicago", "chicago_adsr_idle.png").crop((0, 20, 240, 158)),
    image("chicago-clean", "chicago_adsr_idle_clean.png").crop((0, 20, 240, 158)),
).getbbox(), "stale SOUND touch followed the ADSR page"

# The graph and numeric readout use the same effective held-step lock, while
# the saved sound remains 51 and the held step is 61 (asserted in scenario).
locked = image("locked", "locked_adsr.png")
assert locked.crop((8, 42, 230, 93)).getbbox(), "locked envelope not drawn"

print("sound matrix: active row, wave states, ADSR edit/grace/timeout PASS")
