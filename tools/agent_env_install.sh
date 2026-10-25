#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Idempotent Cloud Agent / host bootstrap for optional Python deps used by
# font generation, build.sh, and host/build_host.sh.
# Prefer installing Pillow. A missing Pillow elsewhere stays not-run in
# tests/run_tests.sh; it is never a cycle stop or a firmware failure.
set -euo pipefail
cd "$(dirname "$0")/.."
PY="${PYTHON:-python3}"
command -v "$PY" >/dev/null
if ! "$PY" -c 'import PIL' >/dev/null 2>&1; then
  "$PY" -m pip install --user Pillow
fi
"$PY" -c 'import PIL; print("Pillow", PIL.__version__)'
