#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# BF-006 Stage 4 host regressions: drum808-exp2 + drum-pan.
# A host pass is not a package pass; packages are built by tools/dev_validate.py.
# Prefer the private worktree cwd from tools/dev_validate.py; fall back to repo root.
set -euo pipefail
if [[ -f tests/run_tests.sh && -d firmware/src/dsp ]]; then
  ROOT="$PWD"
else
  ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
  cd "$ROOT"
fi
export CC="${CC:-cc}" CXX="${CXX:-c++}"
# drum-pan needs build/gen headers (same prerequisite as Stage 3 host-unit).
mkdir -p build/gen
python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h
export X0X_TESTS="drum808-exp2 drum-pan"
exec sh tests/run_tests.sh
