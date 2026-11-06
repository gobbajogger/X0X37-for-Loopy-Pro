#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${X0X_RECOMPILE_OUT:-build/host/control-recompile}
mkdir -p "$OUT"
. tests/host/original_build.sh
case "${X0X_RECOMPILE_EXACT:-1}" in 1) ;; *) echo "Exact FMA required" >&2; exit 2 ;; esac
compile() {
    ${CC:-cc} -O2 -g -std=c99 -D_POSIX_C_SOURCE=200809L -DX0X_HOST $ORIGINAL_FLAGS ${SAN_FLAGS:-} -Ifirmware/src -Ifirmware/src/dsp "$@"
}
# Rename only the defining object; keep all caller-side width work observable.
compile -Doriginal_pw_width=test_real_original_pw_width -c experiments/original_synth/oscillator_motion.c -o "$OUT/oscillator_motion.o"
# This focused probe needs only the synth closure, not app/UI sources.
sources=$(python3 - <<'PY'
import ast
from pathlib import Path
values={}
for node in ast.parse(Path('tools/build.py').read_text()).body:
    if isinstance(node,ast.Assign) and isinstance(node.targets[0],ast.Name) and node.targets[0].id in ('ORIGINAL_SYNTH_UNITS','ORIGINAL_SYNTH_BACKENDS'):
        values[node.targets[0].id]=ast.literal_eval(node.value)
print(' '.join(s for k in ('ORIGINAL_SYNTH_UNITS','ORIGINAL_SYNTH_BACKENDS') for s in values[k] if s!='experiments/original_synth/oscillator_motion.c'))
PY
)
compile tests/host/control_recompile_test.c $sources "$OUT/oscillator_motion.o" firmware/src/dsp/x0x_param.c -lm -o "$OUT/test"
"$OUT/test"
