#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# BF-007 host regression: compile tests/host source; -Itests/host finds project_stage_test.c.
set -e
cd "$(dirname "$0")/../.."
case "$(uname -s)" in
    Darwin) GC=-Wl,-dead_strip ;;
    *) GC=-Wl,--gc-sections ;;
esac
mkdir -p build/host build/gen
[ -f build/gen/x0x_drum_tables.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
[ -f build/gen/x0x_break_bank.h ] || sh tools/gen_builtin_break.sh build/gen >/dev/null
${CC:-cc} -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror -Wno-unused-function ${SAN_FLAGS:-} \
    -ffunction-sections -fdata-sections -DX0X_HOST -DBOARD_SMK37=1 \
    -Ibuild/gen -Ifirmware/src -Itests/host \
    tests/host/bugfinder_legacy_bdpan_motion_test.c \
    firmware/src/app/engine.c firmware/src/app/chicago_param_tables.c \
    firmware/src/app/smc_fx_map_tables.c \
    firmware/src/seq/pattern.c firmware/src/seq/tb3po.c \
    firmware/src/seq/sequencer.c firmware/src/seq/motion.c \
    firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c \
    firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
    "$GC" -lm -o build/host/bugfinder_legacy_bdpan_motion_test
build/host/bugfinder_legacy_bdpan_motion_test
