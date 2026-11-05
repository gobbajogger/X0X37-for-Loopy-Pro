#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-/tmp/fxbus_idle_identity}; mkdir -p "$OUT"
python3 tools/gen_drum_samples.py "$OUT/x0x_drum_samples.h" > "$OUT/generate.log" 2>&1
python3 - "$OUT/old.c" <<'PY'
import re,sys,subprocess
from pathlib import Path
s=subprocess.check_output(['git','show','e9e6643e612b9fb4e968a77d016f3bb24213dc88:firmware/src/dsp/fxbus.c'],text=True)
for name in re.findall(r'^\w[\w *]*\b(fxbus_\w+)\(',s,re.M):
    s=re.sub(r'\b'+name+r'\b','old_'+name,s)
Path(sys.argv[1]).write_text(s)
PY
for flags in '-O2' '-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'; do
    ${CC:-cc} -std=c99 $flags -Wall -Wextra -fno-fast-math -ffp-contract=off -I"$OUT" -Ifirmware/src/dsp -Ifirmware/src tests/host/fxbus_idle_identity_test.c firmware/src/dsp/fxbus.c "$OUT/old.c" firmware/src/dsp/drum909.c firmware/src/dsp/x0x_param.c -lm -o "$OUT/test"
    "$OUT/test"
done
