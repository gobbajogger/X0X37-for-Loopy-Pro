#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/host/drone-unavailable-pattern}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
sh host/build_host.sh --generate-only > "$OUT/assets.log" 2>&1
. tests/host/original_build.sh
CONTROL_FLAGS=
[ "${DRONE_PATTERN_CONTROL_ONLY:-0}" != 1 ] || CONTROL_FLAGS=-DDRONE_PATTERN_CONTROL_ONLY
MODES=${DRONE_PATTERN_MODES:-"normal sanitized"}
set --
[ -z "${DRONE_PATTERN_REGRESSION:-}" ] || set -- "$DRONE_PATTERN_REGRESSION"
for mode in $MODES; do
    case "$mode" in normal|sanitized) ;; *) echo "Unsupported mode: $mode" >&2; exit 2;; esac
    FLAGS='-O2'
    [ "$mode" != sanitized ] || FLAGS='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
    EXE="$OUT/$mode"
    ${CC:-cc} -std=c99 $FLAGS $CONTROL_FLAGS -D_POSIX_C_SOURCE=200809L -DX0X_HOST \
        -Wall -Wextra -Wno-unused-function -Wno-unused-parameter $ORIGINAL_FLAGS \
        -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
        tests/host/drone_unavailable_pattern_test.c $ORIGINAL_SOURCES -lm -o "$EXE" \
        > "$EXE.compile.log" 2>&1 || { cat "$EXE.compile.log"; exit 1; }
    "$EXE" "$@" > "$EXE.log" 2>&1 || { cat "$EXE.log"; exit 1; }
    cat "$EXE.log"
done
[ "$MODES" != 'normal sanitized' ] || cmp "$OUT/normal.log" "$OUT/sanitized.log"
