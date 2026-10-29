#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Real scanner, command queue, UI clear path and original engine regression.
set -eu
TEST_ROOT=$(CDPATH= cd "$(dirname "$0")/../.." && pwd)
SRC=${X0X_SOURCE_ROOT:-$TEST_ROOT}
OUT=${1:?fresh output directory}
sh "$SRC/host/build_host.sh" --generate-only
exec python3 "$TEST_ROOT/tests/host/original_wheels_integration_test.py" --source "$SRC" --out "$OUT"
