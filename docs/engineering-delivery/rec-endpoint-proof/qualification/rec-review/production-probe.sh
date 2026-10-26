#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Product recorder regression; the unmodified d633833a baseline fails the new contract.
set -eu
cd "/Users/mac/Documents/Codex/2026-10-26/task-6/rec-check"
mkdir -p build/gen build/host
sh host/build_host.sh --generate-only >/dev/null
probe_out=${REC_PROBE_OUT:-/Users/mac/Documents/Codex/2026-10-26/task-6/rec-review/production-probe}
${CC:-cc} ${REC_PROBE_FLAGS:-} -O2 -ffp-contract=off -std=c99 -w -DX0X_HOST -DBOARD_SMK37=1 -DX0X_WOBBLE=1 -DX0X_DEV_WOBBLE=1 \
 -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -o "$probe_out" \
 /Users/mac/Documents/Codex/2026-10-26/task-6/rec-review/production-probe.c firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c \
 firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
 firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c \
 firmware/src/app/engine.c experiments/wobble/wobble.c -lm
X0X_HOST_LOAD=0 "$probe_out"
