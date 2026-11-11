#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Real-linked M13 rate regression; optional --legacy-trace for base comparison.
set -eu
cd "$(dirname "$0")/../.."
out=$(mktemp -d "${TMPDIR:-/tmp}/seq-eighth-quarter.XXXXXX")
trap 'rm -rf "$out"' EXIT HUP INT TERM
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror \
    tests/host/seq_eighth_quarter_test.c firmware/src/seq/sequencer.c \
    firmware/src/seq/pattern.c firmware/src/seq/tb3po.c -o "$out/test"
"$out/test" "$@"
