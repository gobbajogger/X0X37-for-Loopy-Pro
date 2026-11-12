#!/usr/bin/env bash
# Scoped whole-BREAK renderer trace; does not certify full root, model or device.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
EXPECTED=fe7272407f98fb90c0c892fdeaeb6b7a4e788454483fc8c0ce47e88da06075a6
for CC in gcc clang; do
    for MODE in optimized sanitizer; do
        if [ "$MODE" = optimized ]; then
            FLAGS=(-O2 -ffp-contract=off)
        else
            FLAGS=(-O1 -g -fno-omit-frame-pointer -ffp-contract=off -fsanitize=address,undefined)
        fi
        "$CC" -std=c11 -Wall -Wextra -Werror -pedantic "${FLAGS[@]}" \
            -I"$ROOT" "$ROOT/tests/host/breaks_renderer_equivalence_test.c" \
            "$ROOT/firmware/src/dsp/breaks.c" -o "$TMP/check"
        ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
            "$TMP/check" --trace "$TMP/trace.pcm" >"$TMP/trace.log" 2>"$TMP/errors.log"
        [ ! -s "$TMP/errors.log" ] || { cat "$TMP/errors.log" >&2; exit 1; }
        [ "$(grep -c '^n=' "$TMP/trace.log")" = 13 ]
        [ "$(wc -c < "$TMP/trace.pcm")" = 18823168 ]
        GOT="$(sha256sum "$TMP/trace.pcm" | cut -d' ' -f1)"
        [ "$GOT" = "$EXPECTED" ] || { echo "BREAK trace mismatch: $CC $MODE $GOT" >&2; exit 1; }
        # Fail closed: the harness must reject an invalid command and a modified trace.
        set +e
        "$TMP/check" --bad >"$TMP/negative.out" 2>&1
        RC=$?
        set -e
        [ "$RC" = 2 ] || { echo "bad-CLI negative not rejected: $RC" >&2; exit 1; }
        cp "$TMP/trace.pcm" "$TMP/corrupt.pcm"
        printf '\001' | dd of="$TMP/corrupt.pcm" bs=1 seek=0 count=1 conv=notrunc 2>/dev/null
        BAD="$(sha256sum "$TMP/corrupt.pcm" | cut -d' ' -f1)"
        [ "$BAD" != "$EXPECTED" ] || { echo "corruption negative not rejected" >&2; exit 1; }
        echo "BREAK_RENDER_PASS $CC/$MODE 13cases 4705792frames PCM_SHA256=$GOT negative_cli=2 negative_corruption=REJECTED"
    done
done
