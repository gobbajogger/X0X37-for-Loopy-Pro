#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
out=$(mktemp -d "${TMPDIR:-/tmp}/break-rate.XXXXXX")
trap 'rm -rf "$out"' EXIT HUP INT TERM
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror \
    tests/host/break_rate_test.c firmware/src/seq/sequencer.c \
    firmware/src/seq/pattern.c firmware/src/seq/tb3po.c -o "$out/test"
"$out/test" "$@"
