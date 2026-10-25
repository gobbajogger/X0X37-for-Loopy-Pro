#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# BF-005 Stage 4 host regressions: sequencer, clk-lock, clock-handover.
# A host pass is not a package pass; packages are built by tools/dev_validate.py.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
export CC="${CC:-cc}" CXX="${CXX:-c++}"
export X0X_TESTS="sequencer clk-lock clock-handover"
exec sh tests/run_tests.sh
