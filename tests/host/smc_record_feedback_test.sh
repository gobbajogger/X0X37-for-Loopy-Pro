#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Compare SMC Record lamps with real panel/SMC-selected recording destinations.
# No target build or hardware access. SMC_SOURCE selects the negative-control source.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c
firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c
firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c"
X0X_HOST_OUT="$OUT/host" sh host/build_host.sh --generate-only > "$OUT/build.log" 2>&1 || { cat "$OUT/build.log"; exit 1; }
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -DX0X_HOST ${X0X_HOST_DEFS:-} -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
    "-DSMC_MIXER_SOURCE=\"${SMC_SOURCE:-app/smc_mixer.c}\"" \
    tests/host/smc_record_feedback_test.c $U -lm -o "$OUT/probe"
"$OUT/probe"
