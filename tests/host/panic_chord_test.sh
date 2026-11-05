#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/x0x-panic-chord.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
# CFLAGS deliberately uses normal compiler flag word splitting.
# shellcheck disable=SC2086
"${CC:-cc}" -std=c99 -Wall -Wextra -Werror -pedantic ${CFLAGS:-} \
    -I"$root/firmware/src" "$root/tests/host/panic_chord_test.c" \
    -o "$test_dir/panic_chord_test"
"$test_dir/panic_chord_test"
