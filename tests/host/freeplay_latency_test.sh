#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Free Play key-to-sound latency on Chicago and Miami: production key route to first sound.
set -eu
cd "$(dirname "$0")/../.."
. tests/host/original_build.sh
OUT=${X0X_LIVE_TEST_OUT:-build/host/freeplay-latency}
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1
X0X_LIVE_TEST_OUT="$OUT" X0X_LIVE_CASES=tests/host/freeplay_latency_cases.inc python3 tests/host/freeplay_live_keys_fixture.py
${CC:-cc} ${SAN_FLAGS:-} -O1 -g -std=c99 -w -DX0X_HOST -DX0X_LIVE_KEY_TEST=1 -DBOARD_SMK37=1 \
    -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen $ORIGINAL_FLAGS \
    "$OUT/probe.c" $ORIGINAL_SOURCES -lm -o "$OUT/test"
"$OUT/test" "$OUT"
