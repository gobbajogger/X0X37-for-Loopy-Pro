#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# BF-007 Stage 4 host regressions: legacy-bdpan-motion + project-stage/protocol/runtime.
# A host pass is not a package pass; packages are built by tools/dev_validate.py.
# Prefer the private worktree cwd from tools/dev_validate.py; fall back to repo root.
set -euo pipefail
if [[ -f tests/run_tests.sh && -d firmware/src/app ]]; then
  ROOT="$PWD"
else
  ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
  cd "$ROOT"
fi
export CC="${CC:-cc}" CXX="${CXX:-c++}"
# Host unit wrappers race on tools/gen_builtin_break.sh when headers are missing
# (Text file busy / Permission denied on build/host/gen_builtin_break). Pre-generate
# like Stage 3 / F10 Stage 4 so parallel X0X_TESTS do not rebuild the same binary.
mkdir -p build/gen build/host
python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
sh tools/gen_builtin_break.sh build/gen >/dev/null
# Parallel host compiles under build/host/iso/<test>/tmp hit intermittent clang
# "unable to rename temporary … No such file or directory". Serialize the suite.
export X0X_TEST_JOBS=1
export X0X_TESTS="legacy-bdpan-motion project-stage project-protocol project-runtime"
exec sh tests/run_tests.sh
