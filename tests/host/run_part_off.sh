#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# M7a per-part ON/OFF on the whole host app (tests/host/part_off_test.c).
#   PART_OFF_BASE_REF=<git commit>  also render the all-ON reference from that untouched tree (bit-exact case)
#   SAN_FLAGS="-fsanitize=address,undefined"  sanitizer build; STRICT=1 adds -Werror
#   PART_OFF_MUTANT=1  build with the gates removed (-DPART_OFF_MUTANT): the gating cases must FAIL
set -eu
cd "$(dirname "$0")/../.."
OUT=${OUT:-build/host/part-off}
mkdir -p "$OUT"
sh host/build_host.sh --generate-only >/dev/null
build() { # <source root> <binary> [extra flags]
    root=$1; bin=$2; shift 2
    ( cd "$root" && . tests/host/original_build.sh &&
      # shellcheck disable=SC2086
      ${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
        ${STRICT:+-Werror} -DX0X_HOST -D_POSIX_C_SOURCE=200809L ${X0X_HOST_DEFS:-} ${SAN_FLAGS:-} "$@" \
        -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
        "$TEST_SRC" $ORIGINAL_FLAGS $ORIGINAL_SOURCES -lm -o "$bin" )
}
TEST_SRC=$(pwd)/tests/host/part_off_test.c
unset PART_OFF_REF_IN || true
if [ -n "${PART_OFF_BASE_REF:-}" ]; then
    rm -rf "$OUT/base" && mkdir -p "$OUT/base"
    git archive "$PART_OFF_BASE_REF" | tar -x -C "$OUT/base"
    ( cd "$OUT/base" && sh host/build_host.sh --generate-only >/dev/null )
    build "$OUT/base" "$(pwd)/$OUT/part_off_ref"
    PART_OFF_REF_OUT=$(pwd)/$OUT/ref.crc "$OUT/part_off_ref"
    export PART_OFF_REF_IN=$(pwd)/$OUT/ref.crc
fi
if [ -n "${PART_OFF_MUTANT:-}" ]; then
    build . "$(pwd)/$OUT/part_off_mutant" -DPART_OFF_MUTANT
    if "$OUT/part_off_mutant"; then echo "part_off: mutant NOT detected"; exit 1; fi
    echo "part_off: mutant detected (gating cases fail as required)"
    exit 0
fi
build . "$(pwd)/$OUT/part_off"
"$OUT/part_off"
