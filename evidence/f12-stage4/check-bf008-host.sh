#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# BF-008 Stage 4 host regressions: fastmath + fastmath-ubsan.
# A host pass is not a package pass; packages are built by tools/dev_validate.py.
# Prefer the private worktree cwd from tools/dev_validate.py; fall back to repo root.
set -euo pipefail
if [[ -f tests/run_tests.sh && -d firmware/src/dsp ]]; then
  ROOT="$PWD"
else
  ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
  cd "$ROOT"
fi
# Prefer gcc for UBSan (default cc is often clang without libclang_rt.ubsan_standalone).
export CC="${CC:-gcc}" CXX="${CXX:-c++}"
mkdir -p build/gen build/host
# Pre-generate headers so parallel/serial suites do not race generators.
if [[ -f tools/gen_drum_samples.py ]]; then
  python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null || true
fi
if [[ -f tools/gen_builtin_break.sh ]]; then
  sh tools/gen_builtin_break.sh build/gen >/dev/null || true
fi
export X0X_TEST_JOBS=1
export X0X_TESTS="fastmath fastmath-ubsan"
exec sh tests/run_tests.sh
