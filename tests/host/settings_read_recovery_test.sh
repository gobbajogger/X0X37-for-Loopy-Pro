#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Boot-read recovery with production project/storage code and actual SMC setter
# case bodies. OUT separates ordinary/sanitized binaries and generated inputs.
set -eu
cd "$(dirname "$0")/../.."
RECOVERY_OUT=${OUT:-build/host/settings-read-recovery}
mkdir -p "$RECOVERY_OUT" build/gen
case "$(uname -s)" in
    Darwin) GC=-Wl,-dead_strip ;;
    *) GC=-Wl,--gc-sections ;;
esac
if [ -n "${CC:-}" ]; then
    RECOVERY_CC=$CC
elif command -v clang >/dev/null 2>&1; then
    RECOVERY_CC=clang
else
    RECOVERY_CC=cc
fi
# Compile the caller cases from the actual composed UI, rather than model them.
python3 - "$RECOVERY_OUT" <<'PY'
from pathlib import Path
import sys
ui = Path('firmware/src/app/ui.c').read_text()
start = ui.index('    case R_SMC_ROLE:\n')
end = ui.index('    case R_FREEPLAY:', start)
Path(sys.argv[1], 'settings_role_calls.inc').write_text(ui[start:end])
PY
ORIGINAL_SRC=$(python3 - <<'PY'
import importlib.util
spec = importlib.util.spec_from_file_location('x0x_build', 'tools/build.py')
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)
print(' '.join([*build.ORIGINAL_SYNTH_UNITS, *build.ORIGINAL_SYNTH_BACKENDS]))
PY
)
[ -f build/gen/x0x_drum_tables.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
[ -f build/gen/x0x_break_bank.h ] || sh tools/gen_builtin_break.sh build/gen >/dev/null
# GCC 14 rejects the vendor weak-inline attribute under -Werror; keep other
# warnings fatal, matching the existing project-stage compiler accommodation.
$RECOVERY_CC -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror -Wno-error=attributes \
    -Wno-unused-function ${SAN_FLAGS:-} -ffunction-sections -fdata-sections \
    -DX0X_HOST -DBOARD_SMK37=1 -DX0X_ORIGINAL_SYNTH=1 -DX0X_WOBBLE=0 -DX0X_DEV_WOBBLE=1 \
    -DORIGINAL_USE_SOFT_FMA -DMIAMI_FX_NO_HOST_FENV \
    -I"$RECOVERY_OUT" -Ibuild/gen -Ifirmware/src -Iexperiments/original_synth -Iexperiments/wobble \
    tests/host/project_settings_recovery_test.c \
    firmware/src/app/engine.c firmware/src/app/chicago_param_tables.c \
    firmware/src/seq/pattern.c firmware/src/seq/tb3po.c \
    firmware/src/seq/sequencer.c firmware/src/seq/motion.c \
    firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c \
    firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
    $ORIGINAL_SRC "$GC" -lm -o "$RECOVERY_OUT/project_settings_recovery_test"
"$RECOVERY_OUT/project_settings_recovery_test"
