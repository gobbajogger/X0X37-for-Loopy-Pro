#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Real production boot/engine/UI; supported original engines only.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
sh host/build_host.sh --generate-only >/dev/null
. tests/host/original_build.sh
U=$ORIGINAL_SOURCES
DEFS=-D_POSIX_C_SOURCE=200809L
[ "$(uname -s)" != Darwin ] || DEFS="$DEFS -D_DARWIN_C_SOURCE"
for mode in normal sanitized; do
    FLAGS='-O2'
    [ "$mode" != sanitized ] || FLAGS='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
    # shellcheck disable=SC2086
    ${CC:-cc} $FLAGS -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
      -DX0X_HOST $DEFS -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
      tests/host/pro_303_entry_crash_test.c $ORIGINAL_FLAGS $U -lm -o "$OUT/$mode"
    for scenario in check-defaults invalid-retained; do
        ASAN_OPTIONS=detect_leaks=0 "$OUT/$mode" "$scenario" >> "$OUT/$mode.log"
    done
done
cmp "$OUT/normal.log" "$OUT/sanitized.log"
cat "$OUT/normal.log"
