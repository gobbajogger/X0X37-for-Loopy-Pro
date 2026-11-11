#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
out=${1:-build/host/chicago-span-hold}
mkdir -p "$out"
for mode in normal sanitized; do
 flags='-Os'
 if [ "$mode" = sanitized ]; then flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'; fi
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -fno-fast-math -ffp-contract=off \
  -Iexperiments/original_synth -Ifirmware/src/app \
  tests/host/chicago_span_hold_test.c experiments/original_synth/chicago_mp4.c -lm \
  -o "$out/$mode"
 "$out/$mode"
done
