#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# M7a per-part ON/OFF on the whole host app (tests/host/part_off_test.c).
#   PART_OFF_BASE_REF=<git commit>  also render the all-ON reference from that untouched tree (bit-exact case)
#   SAN_FLAGS="-fsanitize=address,undefined"  sanitizer build; STRICT=1 adds -Werror
#   PART_OFF_MUTANT=1  build with the gates removed (-DPART_OFF_MUTANT): the gating cases must FAIL
#   PART_OFF_REQUIRE_BASE=1 (tests/run_tests.sh): a base ref that cannot be resolved FAILs (exit 1)
#   instead of NOT_RUN. 4b240bd is a local root commit, absent from other clones.
#   With the base: the storage across firmware (tests/host/part_off_store.c, older = the base's build)
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
if [ -n "${PART_OFF_REQUIRE_BASE:-}" ] && { [ -z "${PART_OFF_BASE_REF:-}" ] ||
        ! git cat-file -e "$PART_OFF_BASE_REF^{commit}" 2>/dev/null; }; then
    echo "  FAIL all_on_bit_exact_vs_base (PART_OFF_REQUIRE_BASE: base '${PART_OFF_BASE_REF:-}' cannot be resolved" \
         "in this clone; fetch or create commit 4b240bd33a0e, or set PART_OFF_BASE_REF to it)"
    exit 1
fi
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
rc=0
"$OUT/part_off" || rc=1
# Storage across firmware: the base (older firmware) and this build on the same flash bytes, compared by sha256
if [ -z "${PART_OFF_BASE_REF:-}" ]; then
    echo "  NOT_RUN store_cross_firmware (PART_OFF_BASE_REF not set)"
else
    S=$(pwd)/$OUT/store; rm -rf "$S"; mkdir -p "$S"
    build_store() { root=$1; bin=$2; src=${3:-part_off_store}
        [ "$root" = . ] || cp tests/host/$src.c "$root/tests/host/$src.c"
        ( cd "$root" && . tests/host/original_build.sh &&
          # shellcheck disable=SC2086
          ${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -w -DX0X_HOST -D_POSIX_C_SOURCE=200809L ${SAN_FLAGS:-} \
            -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
            tests/host/$src.c $ORIGINAL_FLAGS $ORIGINAL_SOURCES -lm -o "$bin" ); }
    build_store "$OUT/base" "$S/old"
    build_store . "$S/new"
    sha() { if [ -f "$1" ]; then sha256sum "$1" | cut -c1-64; else echo missing; fi; }
    kv() { sed -n "s/^$2=//p" "$1"; }
    sfail=0
    verdict() { if [ "$2" = 1 ]; then echo "  PASS $1"; else echo "  FAIL $1"; sfail=$((sfail + 1)); fi; }
    # 1. older firmware on a part OFF project: refused, nothing written, the bytes as they were
    ok=1
    for m in 1 2 4 8 16 5 31; do
        "$S/new" write $m "$S/n$m.bin" >"$S/n$m.write.txt" || true
        "$S/old" boot "$S/n$m.bin" "$S/n$m.old.bin" >"$S/n$m.old.txt" || true
        b0=$(sha "$S/n$m.bin" 2>/dev/null || echo none); b1=$(sha "$S/n$m.old.bin" 2>/dev/null || echo none)
        echo "    mask $m: old load.rc=$(kv "$S/n$m.old.txt" load.rc) unsupported=$(kv "$S/n$m.old.txt" load.unsupported)" \
             "writes load/save/autosave=$(kv "$S/n$m.old.txt" load.writes)/$(kv "$S/n$m.old.txt" save.writes)/$(kv "$S/n$m.old.txt" autosave.writes)" \
             "sha before $b0 after $b1"
        { [ "$(kv "$S/n$m.old.txt" load.rc)" = -9 ] && [ "$(kv "$S/n$m.old.txt" load.unsupported)" = 1 ] &&
          [ "$(kv "$S/n$m.old.txt" load.writes)$(kv "$S/n$m.old.txt" save.writes)$(kv "$S/n$m.old.txt" autosave.writes)" = 000 ] &&
          [ "$b0" = "$b1" ] && [ "$(kv "$S/n$m.write.txt" write.set_len)" = 48 ]; } || ok=0
    done
    verdict old_fw_refuses_part_off_project_bytes_kept $ok
    # 2. every part ON: the same bytes the older firmware saves for the same project
    "$S/new" write 0 "$S/n0.bin" >"$S/n0.write.txt" || true; "$S/old" write 0 "$S/o0.bin" >"$S/o0.write.txt" || true
    echo "    all ON: new $(sha "$S/n0.bin") old $(sha "$S/o0.bin") set_len $(kv "$S/n0.write.txt" write.set_len)/$(kv "$S/o0.write.txt" write.set_len)"
    if cmp -s "$S/n0.bin" "$S/o0.bin"; then verdict all_on_bytes_identical_to_old_fw 1; else verdict all_on_bytes_identical_to_old_fw 0; fi
    # 3. this firmware on the older firmware's project: loads, every part ON
    "$S/new" boot "$S/o0.bin" "$S/o0.new.bin" >"$S/o0.new.txt" || true
    ok=0
    [ "$(kv "$S/o0.new.txt" load.rc)" = 0 ] && [ "$(kv "$S/o0.new.txt" load.user_project)" = 1 ] &&
        [ "$(kv "$S/o0.new.txt" load.mask)" = 0 ] && [ "$(kv "$S/o0.new.txt" load.unsupported)" = 0 ] && ok=1
    verdict new_fw_loads_old_project_all_on $ok
    # 4. round trips, every mask: new -> old (boot, SAVE, AUTOSAVE) -> new, and old -> new (mask m, SAVE) -> old
    ok1=1; ok2=1; m=0
    while [ $m -lt 32 ]; do
        "$S/new" write $m "$S/r$m.bin" >/dev/null || true
        "$S/old" boot "$S/r$m.bin" "$S/r$m.o.bin" >"$S/r$m.o.txt" || true
        "$S/new" boot "$S/r$m.o.bin" "$S/r$m.on.bin" >"$S/r$m.on.txt" || true
        if [ $m = 0 ]; then
            { [ "$(kv "$S/r$m.o.txt" load.rc)" = 0 ] && [ "$(kv "$S/r$m.o.txt" load.user_project)" = 1 ]; } || ok1=0
        else
            { [ "$(kv "$S/r$m.o.txt" load.rc)" = -9 ] && cmp -s "$S/r$m.bin" "$S/r$m.o.bin"; } || ok1=0
        fi
        { [ "$(kv "$S/r$m.on.txt" load.rc)" = 0 ] && [ "$(kv "$S/r$m.on.txt" load.mask)" = $m ] &&
          [ "$(kv "$S/r$m.on.txt" load.user_project)" = 1 ]; } || { ok1=0; echo "    new->old->new mask $m: $(tr '\n' ' ' < "$S/r$m.on.txt")"; }
        "$S/new" setmask "$S/o0.bin" $m "$S/q$m.bin" >"$S/q$m.n.txt" || true
        "$S/old" boot "$S/q$m.bin" "$S/q$m.o.bin" >"$S/q$m.o.txt" || true
        if [ $m = 0 ]; then
            { cmp -s "$S/q$m.bin" "$S/o0.bin" && [ "$(kv "$S/q$m.o.txt" load.rc)" = 0 ] &&
              [ "$(kv "$S/q$m.o.txt" load.user_project)" = 1 ]; } || ok2=0
        else
            { [ "$(kv "$S/q$m.n.txt" setmask.save_rc)" = 0 ] && [ "$(kv "$S/q$m.o.txt" load.rc)" = -9 ] &&
              [ "$(kv "$S/q$m.o.txt" save.writes)$(kv "$S/q$m.o.txt" autosave.writes)" = 00 ] &&
              cmp -s "$S/q$m.bin" "$S/q$m.o.bin"; } || { ok2=0; echo "    old->new->old mask $m: $(tr '\n' ' ' < "$S/q$m.o.txt")"; }
        fi
        m=$((m + 1))
    done
    verdict roundtrip_new_old_new_all_masks $ok1
    verdict roundtrip_old_new_old_all_masks $ok2
    # 5. .x0x files across firmware (tests/host/part_off_file.c): header word 28 carries the OFF mask
    build_store "$OUT/base" "$S/fold" part_off_file
    build_store . "$S/fnew" part_off_file
    "$S/fnew" export 0 "$S/f0.x0x" >/dev/null || true; "$S/fold" export 0 "$S/fo0.x0x" >/dev/null || true
    echo "    all ON file: new $(sha "$S/f0.x0x") old $(sha "$S/fo0.x0x")"
    if cmp -s "$S/f0.x0x" "$S/fo0.x0x"; then verdict all_on_file_identical_to_old_fw 1; else verdict all_on_file_identical_to_old_fw 0; fi
    "$S/fnew" import "$S/fo0.x0x" "$S/fo0.new.x0x" >"$S/fo0.new.txt" || true
    ok=0
    [ "$(kv "$S/fo0.new.txt" import.rc)" = 0 ] && [ "$(kv "$S/fo0.new.txt" import.mask)" = 0 ] &&
        [ "$(kv "$S/fo0.new.txt" import.user_project)" = 1 ] && cmp -s "$S/fo0.x0x" "$S/fo0.new.x0x" && ok=1
    verdict new_fw_imports_old_file_all_on $ok
    ok1=1; ok2=1; m=0
    while [ $m -lt 32 ]; do
        "$S/fnew" export $m "$S/f$m.x0x" >/dev/null || true
        "$S/fnew" import "$S/f$m.x0x" "$S/f$m.n.x0x" >"$S/f$m.n.txt" || true
        "$S/fold" import "$S/f$m.x0x" "$S/f$m.o.x0x" >"$S/f$m.o.txt" || true
        { [ "$(kv "$S/f$m.n.txt" import.rc)" = 0 ] && [ "$(kv "$S/f$m.n.txt" import.mask)" = $m ] &&
          [ "$(kv "$S/f$m.n.txt" import.user_project)" = 1 ] && cmp -s "$S/f$m.x0x" "$S/f$m.n.x0x"; } ||
            { ok1=0; echo "    file new->new mask $m: $(tr '\n' ' ' < "$S/f$m.n.txt")"; }
        if [ $m = 0 ]; then
            { [ "$(kv "$S/f$m.o.txt" import.rc)" = 0 ] && [ "$(kv "$S/f$m.o.txt" import.user_project)" = 1 ]; } || ok2=0
        else
            [ $m = 1 ] || [ $m = 5 ] || [ $m = 31 ] && echo "    file mask $m: old import rc=$(kv "$S/f$m.o.txt" import.rc) writes=$(kv "$S/f$m.o.txt" import.writes) live_replaced=$(kv "$S/f$m.o.txt" import.user_project) sha $(sha "$S/f$m.x0x")"
            { [ "$(kv "$S/f$m.o.txt" import.rc)" = -1 ] && [ "$(kv "$S/f$m.o.txt" import.writes)" = 0 ] &&
              [ "$(kv "$S/f$m.o.txt" import.user_project)" = 0 ]; } || { ok2=0; echo "    file old mask $m: $(tr '\n' ' ' < "$S/f$m.o.txt")"; }
        fi
        m=$((m + 1))
    done
    verdict file_roundtrip_new_all_masks $ok1
    verdict old_fw_refuses_part_off_file $ok2
    ( cd "$S" && sha256sum ./*.x0x ) > "$S/SHA256SUMS.x0x" 2>/dev/null || true
    ( cd "$S" && sha256sum ./*.bin ) > "$S/SHA256SUMS" 2>/dev/null || true
    echo "part_off_store: $([ $sfail = 0 ] && echo ok || echo FAIL) ($sfail failing cases; dumps and sha256 in $OUT/store)"
    [ $sfail = 0 ] || rc=1
fi
exit $rc
