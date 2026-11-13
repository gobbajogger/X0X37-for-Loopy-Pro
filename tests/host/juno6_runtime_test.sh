#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/host/juno6-runtime};mkdir -p "$OUT"
. tests/host/original_build.sh
# Runtime-only integration: no drum/UI generated assets or whole engine needed.
SOURCES=$(python3 -c "import tools.build as b; print(' '.join(b.ORIGINAL_SYNTH_UNITS+b.ORIGINAL_SYNTH_BACKENDS))")
${CC:-cc} -std=c99 -O2 -ffunction-sections -fdata-sections -DX0X_HOST $ORIGINAL_FLAGS ${SAN_FLAGS:-} -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen tests/host/juno6_runtime_test.c $SOURCES -Wl,--gc-sections -lm -o "$OUT/test"
"$OUT/test"
