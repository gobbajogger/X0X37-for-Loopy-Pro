#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Free Play: every one of the 37 SMK-37 keys (central 27 and the 10 outer edge keys),
# at every octave, gives a note-on on each synth engine (Chicago, Miami).
set -eu
cd "$(dirname "$0")/../.."
. tests/host/original_build.sh
OUT=${X0X_LIVE_TEST_OUT:-build/host/freeplay-live-keys}
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1
X0X_LIVE_TEST_OUT="$OUT" python3 tests/host/freeplay_live_keys_fixture.py
${CC:-cc} ${SAN_FLAGS:-} -O1 -g -std=c99 -w -DX0X_HOST -DX0X_LIVE_KEY_TEST=1 -DBOARD_SMK37=1 \
    -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen $ORIGINAL_FLAGS \
    "$OUT/probe.c" $ORIGINAL_SOURCES -lm -o "$OUT/test"
"$OUT/test" "$OUT"
