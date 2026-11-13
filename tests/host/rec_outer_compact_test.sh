#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Exact compact bit-map and source-level REC edge state equivalence.
set -eu
cd "$(dirname "$0")/../.."
OUT=${X0X_REC_COMPACT_OUT:-build/host/rec-outer-compact}
mkdir -p "$OUT"
${CC:-cc} ${SAN_FLAGS:-} -std=c99 -O2 -Wall -Wextra -Werror \
    -Ifirmware/src/app tests/host/rec_outer_compact_test.c -o "$OUT/test"
"$OUT/test"
