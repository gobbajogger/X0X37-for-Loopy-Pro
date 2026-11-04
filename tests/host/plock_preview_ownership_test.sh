#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Step previews and physical keys retain independent runtime ownership.
# Run from any directory. This intentionally excludes calibration case11.
set -eu
cd "$(dirname "$0")/../.."
. tests/host/original_build.sh
OUT=${X0X_PLOCK_OWNERSHIP_OUT:-build/host/plock-preview-ownership}
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
mkdir -p "$OUT/objects" "$OUT/matrix" "$OUT/ownership" "$OUT/prior"
sh host/build_host.sh --generate-only > "$OUT/assets.log" 2>&1
FLAGS='-O1 -g -std=c99 -w -DX0X_HOST -DX0X_LIVE_KEY_TEST=1 -DBOARD_SMK37=1 -DX0X_DEV_PADCAL=0 -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie'
for source in $ORIGINAL_SOURCES; do
    target="$OUT/objects/$(printf '%s' "$source" | tr '/' '_').o"
    ${CC:-cc} $FLAGS -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen $ORIGINAL_FLAGS -c "$source" -o "$target"
done
for fixture in ownership prior; do
    X0X_LIVE_TEST_OUT="$OUT/$fixture" X0X_LIVE_CASES="$PWD/tests/host/plock_preview_${fixture}_cases.inc" python3 tests/host/freeplay_live_keys_fixture.py
    ${CC:-cc} $FLAGS -no-pie -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen $ORIGINAL_FLAGS "$OUT/$fixture/probe.c" "$OUT"/objects/*.o -lm -o "$OUT/$fixture/probe"
done
export ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1
printf 'bank\tvoice\tphysical\tmode\texit\n' > "$OUT/matrix/results.tsv"
failed=0
for bank in 0 1; do
  for voice in 0 1; do
    for key in 17 0 36; do
      for mode in preview held early samepitch retrigger edits; do
        rc=0
        "$OUT/ownership/probe" "$bank" "$voice" "$key" "$mode" > "$OUT/matrix/$bank-$voice-$key-$mode.log" 2>&1 || rc=$?
        printf '%s\t%s\t%s\t%s\t%s\n' "$bank" "$voice" "$key" "$mode" "$rc" >> "$OUT/matrix/results.tsv"
        test "$rc" -eq 0 || failed=$((failed+1))
      done
    done
  done
done
printf 'cases=72 failed=%s\n' "$failed" | tee "$OUT/matrix/summary.txt"
printf 'mode\texit\n' > "$OUT/prior/results.tsv"
for mode in basic ownership navigation central outer; do
    rc=0
    "$OUT/prior/probe" "$mode" > "$OUT/prior/$mode.log" 2>&1 || rc=$?
    printf '%s\t%s\n' "$mode" "$rc" >> "$OUT/prior/results.tsv"
    test "$rc" -eq 0 || failed=$((failed+1))
done
printf 'total_failed=%s\n' "$failed" | tee "$OUT/summary.txt"
test "$failed" -eq 0
