#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Tempo-rate regression and sanitizer coverage; no private audio inputs.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
for mode in normal sanitized; do
 flags='-O2'
 if [ "$mode" = sanitized ]; then flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'; fi
 ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -Wno-error=attributes -ffp-contract=off -DX0X_SYNTH_HOST_TEST -Ifirmware/src tests/host/miami_tempo_test.c experiments/wobble/wobble.c firmware/src/dsp/bass303.c -lm -o "$OUT/$mode"
 "$OUT/$mode"
done
