#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Native only: exact shared decoder correctness; no firmware, hardware, or install.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
for CC in gcc clang; do
    for MODE in optimized sanitizer; do
        if [[ "$MODE" == optimized ]]; then FLAGS=(-O2 -ffp-contract=off); else FLAGS=(-O1 -g -fno-omit-frame-pointer -ffp-contract=off -fsanitize=address,undefined); fi
        "$CC" -std=c11 -Wall -Wextra -Werror -pedantic "${FLAGS[@]}" "$ROOT/tests/host/ima_delta_lut_exact_test.c" -o "$TMP/check"
        ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$TMP/check" --check
        set +e
        "$TMP/check" --invalid >"$TMP/invalid.log" 2>&1
        RESULT=$?
        set -e
        if [[ "$RESULT" -ne 2 ]]; then echo "unexpected invalid-command exit: $RESULT" >&2; exit 1; fi
        grep -q 'usage:' "$TMP/invalid.log" || { cat "$TMP/invalid.log" >&2; exit 1; }
    done
done
