#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Product recorder regression; the unmodified d633833a baseline fails the new contract.
set -eu
cd "$(dirname "$0")/../.."
. tests/host/original_build.sh
mkdir -p build/gen build/host
sh host/build_host.sh --generate-only >/dev/null
probe_out=${REC_PROBE_OUT:-build/host/rec_endpoint_lifecycle_test}
${CC:-cc} ${REC_PROBE_FLAGS:-} -O2 -ffp-contract=off -std=c99 -w -DX0X_HOST -DBOARD_SMK37=1   \
 -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -o "$probe_out" \
 tests/host/rec_endpoint_lifecycle_test.c    \
 $ORIGINAL_FLAGS $ORIGINAL_SOURCES    \
    -lm
X0X_HOST_LOAD=0 "$probe_out"
