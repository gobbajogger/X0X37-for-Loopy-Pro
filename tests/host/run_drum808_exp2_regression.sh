#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Compare exponent-helper float bits to trusted baseline; sanitize the fix.
# Usage: CC=cc tests/host/run_drum808_exp2_regression.sh [BASE_REF]
set -eu
cd "$(dirname "$0")/../.."
BASE=${1:-2695701fe9ae63dbb6f1f76d4dc11ebbf39da834}
CC=${CC:-cc}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
git show "$BASE:firmware/src/dsp/drum808.c" > "$TMP/baseline.c"
for kind in baseline candidate; do
    src="$TMP/baseline.c"
    flags=''
    if [ "$kind" = candidate ]; then
        src="$PWD/firmware/src/dsp/drum808.c"
        flags='-fsanitize=undefined -fno-sanitize-recover=all'
    fi
    "$CC" -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror $flags \
        -I"$PWD/firmware/src/dsp" -DSOURCE=\""$src"\" \
        tests/host/drum808_exp2_regression.c -lm -o "$TMP/$kind"
    "$TMP/$kind" > "$TMP/$kind.bin"
done
cmp "$TMP/baseline.bin" "$TMP/candidate.bin"
echo 'PASS: 254 exact powers and 1053439 bit-identical helper outputs; candidate UBSan clean'
