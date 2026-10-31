#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Orchestrator-only build/render entry point. Does not run assertion suites.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:?output directory required}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
. tests/host/original_build.sh
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1
X0X_LIVE_TEST_OUT="$OUT" X0X_LIVE_CASES=tests/host/storage_warning_capture_cases.inc \
    python3 tests/host/freeplay_live_keys_fixture.py
${CC:-cc} ${SAN_FLAGS:-} -O1 -g -ffp-contract=off -std=c99 -w \
    -DX0X_HOST -DX0X_LIVE_KEY_TEST=1 -DBOARD_SMK37=1 \
    -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen $ORIGINAL_FLAGS \
    "$OUT/probe.c" $ORIGINAL_SOURCES -lm -o "$OUT/capture"
"$OUT/capture" "$OUT"
