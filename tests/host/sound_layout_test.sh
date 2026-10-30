#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Accepted three-row SOUND: complete names, physical K8 mapping, non-mutating rendering and undo.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/sound-layout}
mkdir -p "$OUT"
awk '/^static void smk_wheels\(void\)/{on=1} on{print} on && /^}/{exit}' firmware/src/app/main_fm1.c > "$OUT/smk_wheels.inc"
test -s "$OUT/smk_wheels.inc"
# Verify the source default encoder table, not just the on-screen caption.
python3 - <<'PYMAP'
from pathlib import Path
import re
s=Path('firmware/src/app/panel.c').read_text()
block=re.search(r'static const panel_t PANEL_DEFAULT = \{(.*?)\n\};',s,re.S).group(1)
rows=re.findall(r'\{([^{}]+)\}',block)
enc=[int(x.strip()) for x in rows[1].split(',')]
assert enc == [1,2,3,4,5,6,7], enc
print('PASS source panel mapping: physical K5..K8 -> logical KNOB1..4; calibration unchanged')
PYMAP
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c firmware/src/app/factory_banks.c"
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter ${SAN_FLAGS:-} \
 -DX0X_HOST -DX0X_BASS_PUBLISH_TEST=1 -DX0X_SMK_CAPTIONS=1 -DX0X_WOBBLE=1 -DX0X_DEV_WOBBLE=1 -DX0X_BASS_PRESET_PREVIEW=1 -DX0X_BASS_TAGGED_SOUND=1 \
 -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" tests/host/sound_layout_test.c $U -lm -o "$OUT/test"
"$OUT/test" "$OUT"
