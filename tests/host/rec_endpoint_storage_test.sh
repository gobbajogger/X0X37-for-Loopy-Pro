#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -e
cd "$(dirname "$0")/../.."
case "$(uname -s)" in
    Darwin) GC=-Wl,-dead_strip ;;
    *) GC=-Wl,--gc-sections ;;
esac
# These firmware headers contain JieLi attributes and weak-inline functions.
# Clang accepts them as written; GCC 14 rejects them under -Werror even when
# the production test logic is valid. Respect an explicitly supplied CC.
if [ -n "${CC:-}" ]; then
    STAGE_CC=$CC
elif command -v clang >/dev/null 2>&1; then
    STAGE_CC=clang
else
    STAGE_CC=cc
fi
mkdir -p build/host build/gen
# Match the shipping Chicago/Miami graph, including the required private DSP units.
ORIGINAL_SRC=$(python3 - <<'PYCODE'
import importlib.util
s = importlib.util.spec_from_file_location('x0x_build', 'tools/build.py')
b = importlib.util.module_from_spec(s)
s.loader.exec_module(b)
print(' '.join([*b.ORIGINAL_SYNTH_UNITS, *b.ORIGINAL_SYNTH_BACKENDS]))
PYCODE
)
[ -f build/gen/x0x_drum_tables.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
[ -f build/gen/x0x_break_bank.h ] || sh tools/gen_builtin_break.sh build/gen >/dev/null
$STAGE_CC -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror -Wno-unused-function ${SAN_FLAGS:-} \
    -ffunction-sections -fdata-sections -DX0X_HOST -DBOARD_SMK37=1 \
    -DX0X_ORIGINAL_SYNTH=1 -DX0X_WOBBLE=0 -DX0X_DEV_WOBBLE=1 \
    -DORIGINAL_USE_SOFT_FMA -DMIAMI_FX_NO_HOST_FENV \
    -Ibuild/gen -Ifirmware/src -Iexperiments/original_synth -Iexperiments/wobble tests/host/rec_endpoint_storage_test.c \
    firmware/src/app/engine.c firmware/src/app/chicago_param_tables.c \
    firmware/src/seq/pattern.c firmware/src/seq/tb3po.c \
    firmware/src/seq/sequencer.c firmware/src/seq/motion.c \
    firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c \
    firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
    $ORIGINAL_SRC "$GC" -lm -o build/host/rec_endpoint_storage_test
build/host/rec_endpoint_storage_test
