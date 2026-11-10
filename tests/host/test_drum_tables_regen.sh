#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Delete x0x_drum_tables.h in a private copy and compile the include through
# tests/run_tests.sh. needs_gen must regenerate the header. The shared header
# in the launching tree is not deleted.
set -eu
cd "$(dirname "$0")/../.."
ROOT=$PWD
if grep -n 'rm -f build/gen/x0x_drum_tables.h' tests/host/test_gen_nonfont_tables.sh >/dev/null 2>&1; then
    echo "FAIL: test_gen_nonfont_tables.sh still deletes shared x0x_drum_tables.h" >&2
    exit 1
fi
awk '/^needs_gen\(\)/,/^}/' tests/run_tests.sh | grep -q 'x0x_drum_tables.h' || {
    echo "FAIL: needs_gen() does not name x0x_drum_tables.h" >&2
    exit 1
}
awk '/^ensure_needs_gen\(\)/,/^}/' tests/run_tests.sh | grep -q 'tools/gen_drum_samples.py' || {
    echo "FAIL: ensure_needs_gen does not use gen_drum_samples.py" >&2
    exit 1
}
T=$(mktemp -d "${TMPDIR:-/tmp}/x0x-drum-tables-regen.XXXXXX")
trap 'rm -rf "$T"' EXIT HUP INT TERM
mkdir -p "$T/tree"
(cd "$ROOT" && tar cf - --exclude=./build --exclude=./.git .) | (cd "$T/tree" && tar xf -)
mkdir -p "$T/tree/build/gen"
rm -f "$T/tree/build/gen/x0x_drum_tables.h"
out=$(cd "$T/tree" && X0X_TEST_JOBS=4 X0X_TESTS=drum-tables-include sh tests/run_tests.sh 2>&1) || {
    printf '%s\n' "$out" | sed 's/^/       /' >&2
    echo "FAIL: drum-tables-include with missing x0x_drum_tables.h" >&2
    exit 1
}
printf '%s\n' "$out" | grep -q 'ok   drum-tables-include' || {
    echo "FAIL: expected drum-tables-include ok" >&2
    printf '%s\n' "$out" | sed 's/^/       /' >&2
    exit 1
}
[ -f "$T/tree/build/gen/x0x_drum_tables.h" ] || {
    echo "FAIL: x0x_drum_tables.h was not regenerated" >&2
    exit 1
}
echo "drum-tables-regen: ok"
