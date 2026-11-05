#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Compare SMC Record lamps with real panel/SMC-selected recording destinations.
# No target build or hardware access. SMC_SOURCE selects the negative-control source.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
. tests/host/original_build.sh   # the original-synth units (bass303.c is gone)
U=$ORIGINAL_SOURCES
X0X_HOST_OUT="$OUT/host" sh host/build_host.sh --generate-only > "$OUT/build.log" 2>&1 || { cat "$OUT/build.log"; exit 1; }
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -DX0X_HOST ${X0X_HOST_DEFS:-} $ORIGINAL_FLAGS -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
    "-DSMC_MIXER_SOURCE=\"${SMC_SOURCE:-app/smc_mixer.c}\"" \
    tests/host/smc_record_feedback_test.c $U -lm -o "$OUT/probe"
"$OUT/probe"
