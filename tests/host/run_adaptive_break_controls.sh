#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Actual BREAK input/UI/MIDI route regression; synthetic samples, no device.
set -eu
cd "$(dirname "$0")/../.."
. tests/host/original_build.sh
mkdir -p build/gen build/host
sh host/build_host.sh --generate-only >/dev/null
python3 tests/host/extract_native_break_service.py
probe_out=${ADAPTIVE_PROBE_OUT:-build/host/adaptive_break_controls_test}
${CC:-cc} ${ADAPTIVE_PROBE_FLAGS:-} -O2 -ffp-contract=off -std=c99 -w -DX0X_HOST -DBOARD_SMK37=1   \
 -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -o "$probe_out" \
 tests/host/adaptive_break_controls_test.c    \
 $ORIGINAL_FLAGS $ORIGINAL_SOURCES    \
    -lm
X0X_HOST_LOAD=0 "$probe_out"
