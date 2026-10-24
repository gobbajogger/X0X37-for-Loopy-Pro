#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Production BREAK adversarial renderer / historical preoptimization bit-exact PCM.
# Host-only: no target build, hardware timing, MIDI NoteOff transport or full-root QA.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
EXPECTED=6a29c3d7296ed6c6fdc8ac573e9199d8c009befb2e1b5ced20d32de5dcb41de6
for CC in gcc clang; do
    for MODE in optimized sanitizer; do
        if [ "$MODE" = optimized ]; then
            FLAGS=(-O2 -ffp-contract=off)
        else
            FLAGS=(-O1 -g -fno-omit-frame-pointer -ffp-contract=off -fsanitize=address,undefined)
        fi
        "$CC" -std=c11 -Wall -Wextra -Werror -pedantic "${FLAGS[@]}" \
            -I"$ROOT" "$ROOT/tests/host/breaks_renderer_adversarial_test.c" \
            "$ROOT/firmware/src/dsp/breaks.c" -o "$TMP/check"
        ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
            "$TMP/check" --trace "$TMP/trace.pcm" >"$TMP/trace.log" 2>"$TMP/errors.log"
        [ ! -s "$TMP/errors.log" ] || { cat "$TMP/errors.log" >&2; exit 1; }
        grep -q '^ADVERSARIAL_TRACE scenarios=20 frames=2334720 audible=2230956 zeros_after_stop=39992 failures=0$' "$TMP/trace.log"
        [ "$(wc -c < "$TMP/trace.pcm")" = 9338880 ]
        GOT="$(sha256sum "$TMP/trace.pcm" | cut -d' ' -f1)"
        [ "$GOT" = "$EXPECTED" ] || { echo "ADVERSARIAL mismatch: $CC $MODE $GOT" >&2; exit 1; }
        set +e
        "$TMP/check" --bogus >"$TMP/negative.log" 2>&1
        RC=$?
        set -e
        [ "$RC" = 2 ] || { echo "bad-selector negative failed: $RC" >&2; exit 1; }
        cp "$TMP/trace.pcm" "$TMP/corrupted.pcm"
        printf '\001' | dd of="$TMP/corrupted.pcm" bs=1 seek=0 count=1 conv=notrunc 2>/dev/null
        BAD="$(sha256sum "$TMP/corrupted.pcm" | cut -d' ' -f1)"
        [ "$BAD" != "$EXPECTED" ] || { echo "corrupted trace accepted" >&2; exit 1; }
        echo "ADVERSARIAL_RENDER_PASS $CC/$MODE scenarios=20 frames=2334720 expected_sha=$GOT cli_negative=2 corrupt_negative=REJECTED"
    done
done
