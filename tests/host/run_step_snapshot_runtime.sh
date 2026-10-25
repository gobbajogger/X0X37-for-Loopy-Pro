#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${OUT:-build/host/step-snapshot-runtime}
mkdir -p "$OUT"
sh host/build_host.sh --generate-only
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c
firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c
firmware/src/seq/pattern.c firmware/src/seq/motion.c experiments/wobble/wobble.c"
for mode in legacy miami; do
    defs=""
    if [ "$mode" = miami ]; then defs="-DX0X_DEV_WOBBLE=1 -DX0X_DEV_PADCAL=1"; fi
    # shellcheck disable=SC2086
    ${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
        -D_DARWIN_C_SOURCE -DX0X_HOST -D_POSIX_C_SOURCE=200809L $defs ${SAN_FLAGS:-} \
        -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
        tests/host/step_snapshot_runtime_test.c $U -lm -o "$OUT/$mode"
    "$OUT/$mode"
done
