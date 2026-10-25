#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# BF-034 Stage 4 host regressions: FakeSMK mute/empty SMP_INFO + F03 slot-probe.
# A host pass is not a package pass; packages are built by tools/dev_validate.py.
# Prefer the private worktree cwd from tools/dev_validate.py; fall back to repo root.
set -euo pipefail
if [[ -f tests/install_test.py && -f tests/install_f03_slot_probe_test.py ]]; then
  ROOT="$PWD"
else
  ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
  cd "$ROOT"
fi
export CC="${CC:-cc}" CXX="${CXX:-c++}"
export X0X_DEV_PADCAL="${X0X_DEV_PADCAL:-1}"
export X0X_TEST_JOBS="${X0X_TEST_JOBS:-1}"
python3 tests/install_f03_slot_probe_test.py
python3 tests/install_test.py
python3 tests/x0x_slots_info_preflight_test.py
python3 tests/install_slots_test.py
