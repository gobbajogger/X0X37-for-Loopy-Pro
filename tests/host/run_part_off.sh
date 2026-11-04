#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# M7a per-part ON/OFF on the whole host app (tests/host/part_off_test.c).
#   PART_OFF_BASE_REF=<git commit>  also render the all-ON reference from that untouched tree (bit-exact case)
#   SAN_FLAGS="-fsanitize=address,undefined"  sanitizer build; STRICT=1 adds -Werror
#   PART_OFF_MUTANT=1  build with the gates removed (-DPART_OFF_MUTANT): the gating cases must FAIL
#   PART_OFF_MUTANTS=1 each mutant in part_off_mutants.py on a copy of the tree: the case it names must FAIL
set -eu
cd "$(dirname "$0")/../.."
OUT=${OUT:-build/host/part-off}
mkdir -p "$OUT"
sh host/build_host.sh --generate-only >/dev/null
build() { # <source root> <binary> [extra flags]: the test is compiled inside <root>, so its whole-app
    # #include "../../host/x0x_host.c" (project.c, ui.c, x0x.h, sequencer.h) comes from that tree, not this one
    root=$1; bin=$2; shift 2
    [ "$root" = . ] || cp "$TEST_SRC" "$root/tests/host/part_off_test.c"
    ( cd "$root" && . tests/host/original_build.sh &&
      # shellcheck disable=SC2086
      ${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
        ${STRICT:+-Werror} -DX0X_HOST -D_POSIX_C_SOURCE=200809L ${X0X_HOST_DEFS:-} ${SAN_FLAGS:-} "$@" \
        -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
        tests/host/part_off_test.c $ORIGINAL_FLAGS $ORIGINAL_SOURCES -lm -o "$bin" )
}
TEST_SRC=$(pwd)/tests/host/part_off_test.c
unset PART_OFF_REF_IN || true
if [ -n "${PART_OFF_BASE_REF:-}" ] && ! git cat-file -e "$PART_OFF_BASE_REF^{commit}" 2>/dev/null; then
    echo "  NOT_RUN all_on_bit_exact_vs_base (base $PART_OFF_BASE_REF not in this clone)"; PART_OFF_BASE_REF=
fi
if [ -n "${PART_OFF_BASE_REF:-}" ]; then
    rm -rf "$OUT/base" && mkdir -p "$OUT/base"
    git archive "$PART_OFF_BASE_REF" | tar -x -C "$OUT/base"
    ( cd "$OUT/base" && sh host/build_host.sh --generate-only >/dev/null )
    build "$OUT/base" "$(pwd)/$OUT/part_off_ref"
    PART_OFF_REF_OUT=$(pwd)/$OUT/ref.crc "$OUT/part_off_ref"
    export PART_OFF_REF_IN=$(pwd)/$OUT/ref.crc
fi
# PART_OFF_MUTANTS=1: every named mutant in part_off_mutants.py, each on its own copy; the case it names must FAIL
if [ -n "${PART_OFF_MUTANTS:-}" ]; then
    survived=0
    for m in $(python3 tests/host/part_off_mutants.py list | cut -d' ' -f1); do
        want=$(python3 tests/host/part_off_mutants.py list | awk -v m="$m" '$1==m{print $2}')
        d=$OUT/mut-$m; rm -rf "$d"; mkdir -p "$d"
        git ls-files -z | tar --null -T - -cf - | tar -x -C "$d"
        mkdir -p "$d/build" && cp -R build/gen "$d/build/gen"
        python3 tests/host/part_off_mutants.py apply "$m" "$d"
        build "$d" "$(pwd)/$d/part_off" >"$d/build.log" 2>&1 || { echo "  MUTANT $m: BUILD FAILED"; survived=1; continue; }
        "$d/part_off" >"$d/run.log" 2>&1 || true
        if grep -q "^  FAIL $want\$" "$d/run.log"; then echo "  MUTANT $m: killed by $want"
        else echo "  MUTANT $m: SURVIVED ($want did not fail)"; survived=1; fi
    done
    [ "$survived" = 0 ] && echo "part_off: all mutants killed" || { echo "part_off: a mutant survived"; exit 1; }
    exit 0
fi
if [ -n "${PART_OFF_MUTANT:-}" ]; then
    build . "$(pwd)/$OUT/part_off_mutant" -DPART_OFF_MUTANT
    if "$OUT/part_off_mutant"; then echo "part_off: mutant NOT detected"; exit 1; fi
    echo "part_off: mutant detected (gating cases fail as required)"
    exit 0
fi
build . "$(pwd)/$OUT/part_off"
"$OUT/part_off"
