#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Compile current EQ plus the immutable base implementation with renamed symbols.
set -eu
cd "$(dirname "$0")/../.."
OUT=${X0X_PEAKING_COEF_OUT:-/tmp/x0x-peaking-coef-cache}
mkdir -p "$OUT"
git show 9231c26285c09356b49b2d9afbcd225377aae5c5:experiments/original_synth/chicago_dynamics.c > "$OUT/reference.c"
CC=${CC:-cc}
FLAGS="-O2 -std=c99 -fno-fast-math -ffp-contract=off -Wall -Wextra -Werror -DORIGINAL_USE_SOFT_FMA -ffunction-sections -fdata-sections -Iexperiments/original_synth"
RENAMES=""
for name in $(rg -o 'original_chicago_(mastercomp|peaking)_[a-z_]+' "$OUT/reference.c" | sort -u); do
    RENAMES="$RENAMES -D$name=old_$name"
done
case "$(uname -s)" in Darwin) GC=-Wl,-dead_strip ;; *) GC=-Wl,--gc-sections ;; esac
set -x
$CC $FLAGS ${EXTRA_CFLAGS:-} $RENAMES -c "$OUT/reference.c" -o "$OUT/reference.o"
$CC $FLAGS ${EXTRA_CFLAGS:-} tests/host/chicago_peaking_coef_cache_test.c experiments/original_synth/chicago_dynamics.c experiments/original_synth/fused_reference.c "$OUT/reference.o" $GC -lm -o "$OUT/test"
"$OUT/test"
