#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/host/drone-admission}
mkdir -p "$OUT" build/gen
OUT=$(cd "$OUT" && pwd)
. tests/host/original_build.sh
sh host/build_host.sh --generate-only > "$OUT/generate.log" 2>&1
# Compile the exact production engine through the fixture's provider mode only;
# retain every other maintained whole-app unit and native backend.
SOURCES=
engines=0
for source in $ORIGINAL_SOURCES; do
 if [ "$source" = firmware/src/app/engine.c ]; then engines=$((engines+1)); else SOURCES="$SOURCES $source"; fi
done
[ "$engines" = 1 ] || { echo 'FAIL production engine source selection' >&2;exit 1; }
for profile in shipping historical; do
 profile_flags=
 [ "$profile" != historical ] || profile_flags=-DX0X_DRONE_RUNTIME_ENABLED=1
 for mode in normal sanitized; do
  flags=-O2
  [ "$mode" != sanitized ] || flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer'
  exe="$OUT/$profile-$mode"
  ${CC:-cc} -std=c99 $flags -w -DX0X_HOST -DBOARD_SMK37=1 $ORIGINAL_FLAGS $profile_flags -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
   -DDRONE_ADMISSION_PROVIDER=1 -c tests/host/drone_admission_test.c -o "$exe.provider.o" > "$exe.compile.log" 2>&1
  ${CC:-cc} -std=c99 $flags -w -DX0X_HOST -DBOARD_SMK37=1 $ORIGINAL_FLAGS $profile_flags -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
   tests/host/drone_admission_test.c "$exe.provider.o" $SOURCES -lm -o "$exe" >> "$exe.compile.log" 2>&1
  ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 X0X_HOST_LOAD=0 "$exe" > "$exe.log" 2>&1
  cat "$exe.log"
 done
 cmp "$OUT/$profile-normal.log" "$OUT/$profile-sanitized.log"
done
