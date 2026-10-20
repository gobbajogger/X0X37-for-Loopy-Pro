#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# Exhaustive controller classic fallback results and LE-only link guard.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
TMP=$(mktemp -d "${TMPDIR:-/tmp}/ctl-classic-test.XXXXXX")
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
"${CC:-cc}" -std=c99 -O2 -Wall -Wextra -Werror -DTEST_MODULES=2 \
    "$ROOT/tests/host/ctl_classic_commands_test.c" -o "$TMP/test"
"$TMP/test"
if "${CC:-cc}" -std=c99 -O2 -Wall -Wextra -Werror -DTEST_MODULES=3 \
    "$ROOT/tests/host/ctl_classic_commands_test.c" -o "$TMP/invalid" > "$TMP/link.log" 2>&1; then
    echo "FAIL: classic+LE unexpectedly linked with LE-only fallback handlers" >&2
    exit 1
fi
grep -q 'ctl_classic_fallback_requires_le_only_module' "$TMP/link.log"
echo "PASS: unsupported classic+LE module fails the link"
