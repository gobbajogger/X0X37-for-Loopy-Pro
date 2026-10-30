#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Actual editor + main-loop handoff, synthetic platform. No package or device IO.
set -eu
cd "$(dirname "$0")/../.."
OUT=${OUT:-build/host/install-snapshot}
mkdir -p "$OUT"
[ -f build/gen/x0x_drum_tables.h ] || sh host/build_host.sh --generate-only >/dev/null
awk '/Project transfers cannot queue an update/{on=1}
     on && /^#if X0X_CDC/{exit} on{print}' firmware/src/app/main_fm1.c > "$OUT/project_updates.inc"
test -s "$OUT/project_updates.inc"
python3 tests/host/extract_native_break_service.py
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable \
    ${SAN_FLAGS:-} -Ibuild/gen -I"$OUT" tests/host/install_snapshot_test.c -o "$OUT/test"
"$OUT/test"
