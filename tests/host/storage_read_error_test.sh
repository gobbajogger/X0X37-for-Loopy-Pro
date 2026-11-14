#!/bin/sh
# Focused RAM-NOR regression; run from any directory with sh.
set -eu
repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
test_tmp=$(mktemp -d "${TMPDIR:-/tmp}/storage-read-error.XXXXXX")
trap 'rm -rf "$test_tmp"' EXIT HUP INT TERM
for board in 0 1; do
    # CFLAGS is intentionally word-split to accept optional sanitizer flags.
    "${CC:-cc}" -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function \
        ${CFLAGS:-} -DBOARD_SMK37="$board" \
        "$repo_root/tests/host/storage_read_error_test.c" -o "$test_tmp/test-$board"
    "$test_tmp/test-$board"
done
