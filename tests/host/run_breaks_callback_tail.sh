#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Optional measured baseline must be a separately authenticated source checkout.
# This test never builds firmware, publishes bytes or claims an MCU deadline.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
if { [ "$#" != 2 ] && [ "$#" != 4 ]; } || [ "$1" != '--baseline-root' ]; then
    printf 'usage: bash tests/host/run_breaks_callback_tail.sh --baseline-root /verified/nightly/checkout [--compiler gcc|clang]\n' >&2
    exit 2
fi
COMPILERS=(gcc clang)
if [ "$#" = 4 ]; then
    if [ "$3" != '--compiler' ] || { [ "$4" != gcc ] && [ "$4" != clang ]; }; then
        echo 'unsupported compiler selector' >&2; exit 2
    fi
    COMPILERS=("$4")
fi
BASE="$(cd "$2" && pwd)"
if [ "$ROOT" = "$BASE" ]; then
    printf 'baseline must be a separate checkout\n' >&2
    exit 2
fi
for src in "$ROOT" "$BASE"; do
    test -f "$src/firmware/src/dsp/breaks.c"
    test -f "$src/firmware/src/dsp/breaks.h"
    test -f "$src/firmware/src/dsp/ima_adpcm.h"
done
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
for cc in "${COMPILERS[@]}"; do
    command -v "$cc" >/dev/null
    for variant in baseline candidate; do
        if [ "$variant" = baseline ]; then src="$BASE"; else src="$ROOT"; fi
        "$cc" -std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror -pedantic \
            -I"$src" "$ROOT/tests/host/breaks_callback_tail_perf_test.c" \
            "$src/firmware/src/dsp/breaks.c" -o "$TMP/$cc-$variant"
        "$TMP/$cc-$variant" --blocks 256 >"$TMP/$cc-$variant.log"
        for invalid in --bogus; do
            set +e
            "$TMP/$cc-$variant" "$invalid" >/dev/null 2>"$TMP/invalid.err"
            rc=$?
            set -e
            test "$rc" = 2 || { echo "$cc $variant invalid argument exit=$rc, expected 2" >&2; exit 1; }
        done
        "$cc" -std=c11 -O1 -g -fno-omit-frame-pointer -ffp-contract=off \
            -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined \
            -I"$src" "$ROOT/tests/host/breaks_callback_tail_perf_test.c" \
            "$src/firmware/src/dsp/breaks.c" -o "$TMP/$cc-san-$variant"
        ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
            "$TMP/$cc-san-$variant" --blocks 64 >"$TMP/$cc-san-$variant.log" 2>"$TMP/$cc-san-$variant.err"
        test ! -s "$TMP/$cc-san-$variant.err" || {
            cat "$TMP/$cc-san-$variant.err" >&2; exit 1;
        }
        printf 'PASS %s %s optimized/sanitizer render, negative CLI exit 2\n' "$cc" "$variant"
    done
    python3 "$ROOT/tests/host/compare_callback_tail.py" \
        "$TMP/$cc-baseline.log" "$TMP/$cc-candidate.log" \
        --output "$TMP/$cc-summary.json"
    python3 "$ROOT/tests/host/compare_callback_tail.py" \
        "$TMP/$cc-san-baseline.log" "$TMP/$cc-san-candidate.log" \
        --output "$TMP/$cc-san-summary.json"
    if [ -n "${X0X_TAIL_LOG_DIR:-}" ]; then
        mkdir -p "$X0X_TAIL_LOG_DIR"
        cp "$TMP/$cc-"*.log "$TMP/$cc-"*.json "$X0X_TAIL_LOG_DIR/"
    fi
done
printf 'PASS host BREAK PCM/control parity, %s O2/ASan/UBSan, negative CLI\n' "${COMPILERS[*]}"
