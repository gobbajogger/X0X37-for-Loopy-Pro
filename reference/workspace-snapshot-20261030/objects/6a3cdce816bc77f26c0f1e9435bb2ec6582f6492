#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Real sequencer sink: audible NOTE/held/wrap positives, orphan negatives.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
${CC:-cc} -O2 -ffp-contract=off -Wall -Wextra \
    tests/host/home_tie_semantics_test.c firmware/src/seq/sequencer.c \
    firmware/src/seq/tb3po.c firmware/src/seq/pattern.c -o "$OUT/test"
"$OUT/test"
