#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Phase 1 mod-wheel readout: names, percent, unassigned-once, frames, notes, cost.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/modwheel-ui}
mkdir -p "$OUT/1x" "$OUT/3x"
awk '/^static void smk_wheels\(void\)/{on=1} on{print} on && /^}/{exit}' firmware/src/app/main_fm1.c > "$OUT/smk_wheels.inc"
test -s "$OUT/smk_wheels.inc"
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1 || { cat "$OUT/assets.log"; exit 1; }
. tests/host/original_build.sh
U=$ORIGINAL_SOURCES
HOST_DEFS='-D_POSIX_C_SOURCE=200809L'
if [ "$(uname -s)" = Darwin ]; then HOST_DEFS="$HOST_DEFS -D_DARWIN_C_SOURCE"; fi
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter ${SAN_FLAGS:-} \
 -DX0X_HOST -DBOARD_SMK37=1 -DX0X_DEV_PADCAL=1 -DX0X_SMK_CAPTIONS=1 \
 -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" \
 $HOST_DEFS $ORIGINAL_FLAGS tests/host/modwheel_ui_test.c $U -lm -o "$OUT/test"
"$OUT/test" "$OUT"
python3 - "$OUT" << 'PY'
import sys, os
from pathlib import Path
out = Path(sys.argv[1])
try:
    from PIL import Image
except ImportError:
    print("FRAME_COMPARE skipped: no Pillow")
    sys.exit(0)

def nn3(im):
    return im.resize((im.width * 3, im.height * 3), Image.Resampling.NEAREST)

d1 = out / "1x"
d3 = out / "3x"
d3.mkdir(parents=True, exist_ok=True)
names = [
    "before-juno-seq.png",
    "after-single-juno.png",
    "after-single-chicago.png",
    "after-single-miami.png",
    "after-single-drone.png",
    "after-rest-0.png",
    "after-double.png",
    "after-unassigned.png",
]
ux = Path("/workspace/smk-ux/overhaul/modwheel/frames")
# box at (8,103) 224x50
BOX = (8, 103, 8 + 224, 103 + 50)
print("FRAME_COMPARE  box=(8,103,224,50)")
for name in names:
    src = d1 / name
    if not src.exists():
        print(f"  MISSING {name}")
        sys.exit(1)
    im = Image.open(src).convert("RGB")
    nn3(im).save(d3 / name)
    gold = ux / "1x" / name
    if not gold.exists():
        print(f"  no UX gold for {name}")
        continue
    g = Image.open(gold).convert("RGB")
    a, b = im.crop(BOX), g.crop(BOX)
    pa, pb = list(a.getdata()), list(b.getdata())
    n = len(pa)
    same = sum(1 for x, y in zip(pa, pb) if x == y)
    # also full frame
    fa, fb_ = list(im.getdata()), list(g.getdata())
    fn = len(fa)
    fsame = sum(1 for x, y in zip(fa, fb_) if x == y)
    print(f"  {name:28s}  box {same}/{n} ({100.0*same/n:.1f}%)  full {fsame}/{fn} ({100.0*fsame/fn:.1f}%)")
print("FRAME_COMPARE done")
PY
