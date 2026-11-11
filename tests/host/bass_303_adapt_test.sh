#!/bin/sh
# 303-style accent/slide/tune helpers (c4ecd248), bit-exact when off.
set -eu
cd "$(dirname "$0")/../.."
OUT="${BASS_303_ADAPT_OUT:-build/host/bass-303-adapt}"
mkdir -p "$OUT"
"${CC:-cc}" -std=c99 -Wall -Wextra -Werror -pedantic -O2 -ffp-contract=off \
    ${SAN_FLAGS:-} -Ifirmware/src \
    tests/host/bass_303_adapt_test.c -lm -o "$OUT/test"
"$OUT/test"
