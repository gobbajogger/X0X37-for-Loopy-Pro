#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Focused sound-reset cases: -Wall -Wextra -Werror, ASan/UBSan, then mutants.
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

expect_mutant() {
    name=$1
    def=$2
    needle=$3
    # shellcheck disable=SC2086
    "$CC" $WARN -O2 -D"$def"=1 $INC -o "$OUT/sound_reset_mutant_$name" tests/host/sound_reset_test.c
    if "$OUT/sound_reset_mutant_$name" >"$OUT/sound_reset_mutant_$name.log" 2>&1; then
        echo "sound-reset mutant $name passed; $needle must fail"
        exit 1
    fi
    grep -q "$needle" "$OUT/sound_reset_mutant_$name.log"
    echo "sound-reset mutant: $name failed as required"
}

expect_mutant pan SOUND_RESET_MUTANT_CLEAR_PAN "mixer pan changed"
expect_mutant last SOUND_RESET_MUTANT_IGNORE_LAST_PARAM "last-parameter stayed NO_SAVED"
expect_mutant tag SOUND_RESET_MUTANT_IGNORE_BASS_TAG "bass-tag stayed NO_SAVED"
expect_mutant restore SOUND_RESET_MUTANT_SKIP_RESTORE "oversize diff kept the factory cells"
"$OUT/sound_reset_test"
