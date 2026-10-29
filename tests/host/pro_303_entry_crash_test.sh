#!/bin/sh
# Production boot/engine/UI regression; both product WBL1 and Chicago-only builds.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
if [ ! -f build/gen/felucca_font.h ] || [ ! -f build/gen/x0x_drum_samples.h ] || \
   [ ! -f build/gen/x0x_drum_tables.h ] || [ ! -f build/gen/x0x_break_bank.h ]; then
    sh host/build_host.sh --generate-only
fi
. tests/host/original_build.sh
U=$ORIGINAL_SOURCES
HOST_DEFS=
if [ "$(uname -s)" = Darwin ]; then HOST_DEFS=-D_DARWIN_C_SOURCE; fi
for wobble in 1 0; do
    ${CC:-cc} -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -ffp-contract=off -std=c99 \
        -Wno-unused-function -Wno-unused-parameter -DX0X_HOST -DX0X_WOBBLE=$wobble -DX0X_DEV_WOBBLE=$wobble \
        $HOST_DEFS -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
        tests/host/pro_303_entry_crash_test.c $ORIGINAL_FLAGS $U -lm -o "$OUT/test"
    "$OUT/test" check-defaults
    "$OUT/test"
    "$OUT/test" invalid-retained
done
