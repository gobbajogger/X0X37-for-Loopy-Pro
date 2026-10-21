#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Compare initialization state and rendered audio with the pre-outline revision.
# Usage: CC=clang tests/host/run_drum909_init_equivalence.sh [BASE_REF]
# BASE_REF must identify a trusted, locally available repository revision.
set -eu
cd "$(dirname "$0")/../.."
BASE=${1:-2695701fe9ae63dbb6f1f76d4dc11ebbf39da834}
CC=${CC:-clang}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
mkdir "$TMP/gen"
git show "$BASE:firmware/src/dsp/drum909.c" > "$TMP/baseline.c"
python3 tools/gen_drum_samples.py "$TMP/gen/x0x_drum_samples.h"
for kind in baseline candidate; do
    src="$TMP/baseline.c"
    [ "$kind" = baseline ] || src="$PWD/firmware/src/dsp/drum909.c"
    "$CC" -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror \
        -I"$PWD/firmware/src/dsp" -I"$TMP/gen" -DSOURCE=\""$src"\" \
        tests/host/drum909_init_equivalence.c -o "$TMP/$kind"
    "$TMP/$kind" > "$TMP/$kind.txt"
done
cmp "$TMP/baseline.txt" "$TMP/candidate.txt"
echo 'PASS: identical initialized state and 264 audio/retrigger cases, 131072 frames each'
