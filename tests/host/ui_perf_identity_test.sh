#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# GCC primitive-call instrumentation is host-only; firmware gets no counters.
set -eu
cd "$(dirname "$0")/../.."
tmp=$(mktemp -d "${TMPDIR:-/tmp}/arc8-identity.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
"${CC:-cc}" -std=c99 -O2 -ffp-contract=off -Ifirmware/src tests/host/gen_arc8_table.c -o "$tmp/gen"
"$tmp/gen" > "$tmp/geometry.h"
cmp "$tmp/geometry.h" firmware/src/app/ui_arc8_geometry.h
SAN_FLAGS="${SAN_FLAGS:-} -finstrument-functions" \
SMC_UI_OUT=${SMC_UI_OUT:-build/host/ui-perf-identity} \
SMC_UI_TEST_FILE=tests/host/ui_perf_identity_test.c sh tests/host/smc_ui_test.sh
