#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Check the real packed cache against the uncached decoder, including cold sentinel.
set -eu
cd "$(dirname "$0")/../.."
OUT=${X0X_PACKED_CACHE_OUT:-build/host/chicago-packed-cache}
mkdir -p "$OUT"
for mode in normal sanitized; do
 flags='-O2'
 if [ "$mode" = sanitized ]; then flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'; fi
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -Iexperiments/original_synth \
  tests/host/chicago_packed_cache_test.c experiments/original_synth/chicago_packed.c \
  experiments/original_synth/lossless_table.c -o "$OUT/$mode"
 "$OUT/$mode"
done
