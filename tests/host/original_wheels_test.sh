#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Narrow scanner-dispatch proof; no DSP, host unity, hardware, or generated assets.
set -eu
TEST_ROOT=$(CDPATH= cd "$(dirname "$0")/../.." && pwd)
SRC=${X0X_SOURCE_ROOT:-$TEST_ROOT}
if [ -n "${X0X_WHEELS_OUT:-}" ]; then
    OUT=$X0X_WHEELS_OUT
    mkdir -p "$OUT"
else
    OUT=$(mktemp -d)
    trap 'rm -rf "$OUT"' EXIT HUP INT TERM
fi
awk '/^static void smk_wheels\(void\)/{on=1} on{print} on && /^}/{exit}' \
    "$SRC/firmware/src/app/main_fm1.c" > "$OUT/smk_wheels.inc"
test -s "$OUT/smk_wheels.inc"
for mode in normal sanitized; do
    if [ "$mode" = normal ]; then
        set -- -O2
    else
        set -- -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all
    fi
    ${CC:-cc} -std=c99 -Wall -Wextra -Werror -pedantic -fno-fast-math -ffp-contract=off \
        -DX0X_HOST=1 -DX0X_ORIGINAL_SYNTH=1 -DX0X_DEV_WOBBLE=0 \
        -I"$SRC/firmware/src" -I"$OUT" "$@" \
        "$TEST_ROOT/tests/host/original_wheels_test.c" -o "$OUT/$mode" \
        > "$OUT/$mode-compile.log" 2>&1 || { cat "$OUT/$mode-compile.log"; exit 1; }
    : > "$OUT/$mode.log"
    for part in 0 1; do
        for kind in 0 1; do
            ASAN_OPTIONS=${ASAN_OPTIONS:-detect_leaks=0} UBSAN_OPTIONS=halt_on_error=1 \
                "$OUT/$mode" "$part" "$kind" >> "$OUT/$mode.log" 2>&1 || {
                cat "$OUT/$mode.log"; exit 1;
            }
        done
    done
    cat "$OUT/$mode.log"
done
diff -u "$OUT/normal.log" "$OUT/sanitized.log"
printf '%s\n' 'PASS original wheels: normal and ASan/UBSan results agree'
