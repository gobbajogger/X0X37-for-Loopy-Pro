#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${X0X_REC37_OWNER_OUT:-build/host/rec37-owner-integration}
mkdir -p "$OUT"
python3 tests/host/rec37_owner_source_test.py "$OUT/probe.c"
${CC:-cc} ${SAN_FLAGS:-} -std=c99 -O1 -g -Wall -Wextra -Werror \
    -Ifirmware/src/app "$OUT/probe.c" -o "$OUT/test"
"$OUT/test"
