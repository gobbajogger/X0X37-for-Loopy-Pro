#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/x0x-play-question.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
# CFLAGS deliberately uses normal compiler flag word splitting.
# shellcheck disable=SC2086
"${CC:-cc}" -std=c99 -Wall -Wextra -Werror -pedantic ${CFLAGS:-} \
    -I"$root/firmware/src" "$root/tests/host/play_question_test.c" \
    -o "$test_dir/play_question_test"
"$test_dir/play_question_test"
