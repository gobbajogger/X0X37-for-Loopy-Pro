#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Product recorder regression; the unmodified d633833a baseline fails the new contract.
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/gen build/host
sh host/build_host.sh --generate-only >/dev/null
probe_out=${REC_PROBE_OUT:-build/host/rec_held_duration_test}
${CC:-cc} ${REC_PROBE_FLAGS:-} -O2 -ffp-contract=off -std=c99 -w -DX0X_HOST -DBOARD_SMK37=1 -DX0X_WOBBLE=1 -DX0X_DEV_WOBBLE=1 \
 -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -o "$probe_out" \
 tests/host/rec_held_duration_test.c firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c \
 firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
 firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c \
 firmware/src/app/engine.c -lm
X0X_HOST_LOAD=0 "$probe_out"
