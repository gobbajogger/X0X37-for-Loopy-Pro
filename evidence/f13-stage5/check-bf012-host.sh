#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# BF-012 Stage 5 host regressions: master suite (limiter OFF→ON look-ahead clear).
set -euo pipefail
if [[ -f tests/run_tests.sh && -d firmware/src/dsp ]]; then
  ROOT="$PWD"
else
  ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
  cd "$ROOT"
fi
export CC="${CC:-cc}" CXX="${CXX:-c++}"
mkdir -p build/gen build/host
python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
sh tools/gen_builtin_break.sh build/gen >/dev/null
export X0X_TEST_JOBS=1
export X0X_TESTS=master
export X0X_DEV_PADCAL=1
exec sh tests/run_tests.sh
