#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# X0X_TESTS=stereo (or drum-pan) alone with empty build/gen must self-fill
# x0x_drum_tables.h. Full-suite / host-build ordering is covered elsewhere;
# filtered alone used to fail with "file not found" when gen_wait skipped.
set -eu
cd "$(dirname "$0")/../.."
T=$(mktemp -d "${TMPDIR:-/tmp}/x0x-stereo-alone.XXXXXX")
trap 'rm -rf "$T"' EXIT HUP INT TERM
fails=0
for name in stereo drum-pan; do
    for jobs in 1 8; do
        rm -rf "$T/tree"
        mkdir -p "$T/tree"
        (cd . && tar cf - --exclude=./build --exclude=./.git .) | (cd "$T/tree" && tar xf -) \
            || { echo "FAIL copy of the tree"; exit 1; }
        mkdir -p "$T/tree/build/gen" "$T/tree/build/host"
        out=$(cd "$T/tree" && X0X_TEST_JOBS=$jobs X0X_TESTS="$name" sh tests/run_tests.sh 2>&1) || {
            printf '%s\n' "$out" | sed 's/^/       /' >&2
            echo "FAIL: $name alone jobs=$jobs (rc non-zero)" >&2
            fails=$((fails + 1))
            continue
        }
        printf '%s\n' "$out" | grep -q "ok   $name" || {
            echo "FAIL: expected ok $name jobs=$jobs" >&2
            printf '%s\n' "$out" | sed 's/^/       /' >&2
            fails=$((fails + 1))
            continue
        }
        [ -f "$T/tree/build/gen/x0x_drum_tables.h" ] || {
            echo "FAIL: $name alone jobs=$jobs left x0x_drum_tables.h missing" >&2
            fails=$((fails + 1))
            continue
        }
        echo "ok   $name alone jobs=$jobs"
    done
done
[ "$fails" = 0 ] && echo "stereo-alone-gen: ok" || { echo "stereo-alone-gen: $fails FAILED"; exit 1; }
