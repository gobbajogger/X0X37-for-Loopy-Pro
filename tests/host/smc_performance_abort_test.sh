#!/bin/sh
# Run production UI, sequencer, engine and SMC queues. Use scratch host output.
# SMC_SOURCE_FILE can select a saved base smc_mixer.c for the failing regression.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
. tests/host/original_build.sh   # the original-synth units (bass303.c is gone)
U=$ORIGINAL_SOURCES
X0X_HOST_OUT="$OUT/host" sh host/build_host.sh --generate-only > "$OUT/build.log" 2>&1 || { cat "$OUT/build.log"; exit 1; }
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -DX0X_HOST $ORIGINAL_FLAGS -DSMC_SOURCE_FILE=\"${SMC_SOURCE_FILE:-app/smc_mixer.c}\" ${X0X_HOST_DEFS:-} \
    -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
    tests/host/smc_performance_abort_test.c $U -lm -o "$OUT/test"
"$OUT/test"
