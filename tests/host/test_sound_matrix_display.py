"""Production 240px B renderer: four-row geometry, waves and transient envelope.
Independent time-law/mapping assertions are in sound_matrix_test.c, not inferred
from nonempty images. These scenarios exercise the normal host input path.
"""
import pathlib
import subprocess
import sys
from PIL import Image, ImageChops
root = pathlib.Path(__file__).resolve().parents[2]
binary = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else root / "build/host/x0x_host_matrix_tagged"
out = pathlib.Path(sys.argv[2]) if len(sys.argv) > 2 else root / "build/ui-matrix-proof"
for scenario, folder in (("sound_matrix_miami", "miami"), ("sound_matrix_chicago", "chicago"),
                         ("sound_matrix_drum", "drum"), ("sound_matrix_chicago_idle", "chicago-clean"),
                         ("sound_matrix_lock", "locked")):
    dest = out / folder
    dest.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(binary), str(root / f"tests/scenarios/{scenario}.x0x"), str(dest)], check=True)
def image(group, name):
    im = Image.open(out / group / name).convert("RGB")
    assert im.size == (240, 240), (group, name, im.size)
    return im
def different(group, a, b, area):
    assert ImageChops.difference(image(group,a).crop(area), image(group,b).crop(area)).getbbox(), (group,a,b,area)
def identical(group, a, b, area):
    assert not ImageChops.difference(image(group,a).crop(area), image(group,b).crop(area)).getbbox(), (group,a,b,area)
# Every primary cell has a visible label/value. Active row spans all four knobs.
for group, name in (("miami","miami_sound.png"),("chicago","chicago_sound.png")):
    im=image(group,name)
    col=im.getpixel((7,48))
    assert col != (0,0,0)
    for row in range(4):
        for knob in range(4):
            x=4+knob*59; y=24+row*50
            assert im.crop((x+6,y+11,x+54,y+46)).getbbox(), (group,row,knob)
            assert (im.getpixel((x+3,y+24))==col)==(row==0), (group,row,knob,"active row")
    assert im.crop((4,174,236,224)).getbbox(), "fourth row absent from lower strip"
# Wave cells follow the actual continuous/discrete controls in their own rows.
for a,b in (("chicago_saw.png","chicago_mix.png"),("chicago_mix.png","chicago_square.png")):
    different("chicago",a,b,(185,48,232,65))
different("miami","miami_triangle.png","miami_square.png",(6,48,58,65))
different("miami","miami_xmod_triangle.png","miami_xmod_pulse.png",(6,98,58,115))
different("miami","miami_wobble_triangle.png","miami_wobble_square.png",(6,148,58,165))
graph=(12,76,229,152)
different("miami","miami_adsr_idle.png","miami_adsr_active.png",graph)
identical("miami","miami_adsr_active.png","miami_adsr_grace.png",graph)
different("miami","miami_adsr_active.png","miami_adsr_sustain.png",graph)
different("miami","miami_adsr_sustain.png","miami_adsr_done.png",graph)
different("chicago","chicago_adsr_idle.png","chicago_adsr_active.png",graph)
different("chicago","chicago_adsr_active.png","chicago_adsr_done.png",graph)
# A SOUND detent cannot leak into the amp overlay after row/bank navigation.
assert not ImageChops.difference(image("chicago","chicago_adsr_idle.png").crop((0,174,240,224)),
                                image("chicago-clean","chicago_adsr_idle_clean.png").crop((0,174,240,224))).getbbox()
# Real lower-strip continuation, then clean next bank and wrap back to row one.
different("miami","miami_adsr_done.png","miami_page2.png",(0,20,240,240))
assert image("miami","miami_wrap.png").getpixel((7,48)) != (0,0,0)
print("sound matrix B: 4x4 geometry, row highlight, wave states, bank/wrap and ADSR lifecycle PASS")
