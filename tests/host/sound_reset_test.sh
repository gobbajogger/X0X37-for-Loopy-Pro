#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Focused sound-reset cases: -Wall -Wextra -Werror, ASan/UBSan, then a pan mutant.
set -eu
cd "$(dirname "$0")/../.."
CC=${CC:-cc}
SAN_CC=${SAN_CC:-gcc}
OUT=${1:-build/host}
mkdir -p "$OUT"
INC="-Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ifirmware/src/seq"
WARN="-std=c99 -Wall -Wextra -Werror"

# shellcheck disable=SC2086
"$CC" $WARN -O2 $INC -o "$OUT/sound_reset_test" tests/host/sound_reset_test.c
"$OUT/sound_reset_test"

# shellcheck disable=SC2086
"$SAN_CC" $WARN -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all $INC \
    -o "$OUT/sound_reset_test_san" tests/host/sound_reset_test.c
"$OUT/sound_reset_test_san"

# shellcheck disable=SC2086
"$CC" $WARN -O2 -DSOUND_RESET_MUTANT_CLEAR_PAN=1 $INC \
    -o "$OUT/sound_reset_mutant" tests/host/sound_reset_test.c
if "$OUT/sound_reset_mutant" >"$OUT/sound_reset_mutant.log" 2>&1; then
    echo "sound-reset mutant passed; mixer pan must fail"
    exit 1
fi
grep -q "mixer pan changed" "$OUT/sound_reset_mutant.log"
echo "sound-reset mutant: mixer pan check failed as required"
"$OUT/sound_reset_test"
