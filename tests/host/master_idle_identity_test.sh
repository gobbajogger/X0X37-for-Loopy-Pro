#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Compare against the actual pinned-base source, with public symbols renamed.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-/tmp/master_idle_identity}; mkdir -p "$OUT"
python3 - "$OUT/old.c" <<'PY'
import re,sys,subprocess
from pathlib import Path
s=subprocess.check_output(['git','show','9231c26285c09356b49b2d9afbcd225377aae5c5:firmware/src/dsp/master.c'],text=True)
for name in re.findall(r'^\w[\w *]*\b(master_\w+)\(',s,re.M):
    s=re.sub(r'\b'+name+r'\b','old_'+name,s)
Path(sys.argv[1]).write_text(s)
PY
for flags in '-O2' '-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'; do
    ${CC:-cc} -std=c99 $flags -Wall -Wextra -fno-fast-math -ffp-contract=off -Ifirmware/src/dsp -Ifirmware/src tests/host/master_idle_identity_test.c firmware/src/dsp/master.c "$OUT/old.c" -lm -o "$OUT/test"
    "$OUT/test"
done
