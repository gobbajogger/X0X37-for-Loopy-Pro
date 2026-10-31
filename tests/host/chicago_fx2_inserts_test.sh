#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Archived Chicago inserts on the compact FX2 owner (host, native + sanitized).
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/chicago_fx2_inserts}; mkdir -p "$OUT"
. tests/host/original_build.sh
EXP=$(printf '%s\n' $ORIGINAL_SOURCES | grep '^experiments/')
for flags in -O2 '-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'; do
    ${CC:-cc} -std=c99 $flags -Wall -Wextra $ORIGINAL_FLAGS -Ifirmware/src -Ifirmware/src/app \
        tests/host/chicago_fx2_inserts_test.c firmware/src/app/original_sound.c $EXP -lm -o "$OUT/t"
    "$OUT/t"
done
