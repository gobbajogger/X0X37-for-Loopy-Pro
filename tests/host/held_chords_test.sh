#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/held-chords}
mkdir -p "$OUT"
W='-std=c11 -O2 -ffp-contract=off -Wall -Wextra -Werror'
${CC:-cc} $W tests/host/seq_held_chord_test.c firmware/src/seq/sequencer.c firmware/src/seq/pattern.c firmware/src/seq/tb3po.c -lm -o "$OUT/seq"
"$OUT/seq"
${CC:-cc} $W tests/host/held_chord_store_test.c firmware/src/seq/motion.c -o "$OUT/store"
"$OUT/store"
