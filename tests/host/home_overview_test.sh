#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Production HOME renderer: content columns, assignment tick, cue and nonmutation.
set -eu
cd "$(dirname "$0")/../.."
OUT=${HOME_OVERVIEW_OUT:-build/host/home-overview}
mkdir -p "$OUT"
sh host/build_host.sh --generate-only > "$OUT/generate.log" 2>&1
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c"
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter ${SAN_FLAGS:-} \
    -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    tests/host/home_overview_test.c $U -lm -o "$OUT/test"
"$OUT/test" "$OUT" "$@"
