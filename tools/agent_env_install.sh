#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Idempotent Cloud Agent / host bootstrap for optional Python deps used by
# font generation, build.sh, and host/build_host.sh.
# Prefer installing Pillow. A missing Pillow elsewhere stays not-run in
# tests/run_tests.sh; it is never a cycle stop or a firmware failure.
# Install failure is nonfatal: report unavailable and exit 0 so environment
# setup continues (pip --user fails in many venvs; set -e must not halt setup).
set -euo pipefail
cd "$(dirname "$0")/.."
PY="${PYTHON:-python3}"
if ! command -v "$PY" >/dev/null; then
  echo "agent_env_install: $PY not found; Pillow unavailable" >&2
  exit 0
fi
if "$PY" -c 'import PIL' >/dev/null 2>&1; then
  "$PY" -c 'import PIL; print("Pillow", PIL.__version__)'
  exit 0
fi
# In a venv, pip rejects --user when user site-packages are unavailable. Try a
# normal install first; fall back to --user for system/agent Pythons.
if ! {
  "$PY" -m pip install Pillow || "$PY" -m pip install --user Pillow
}; then
  echo "agent_env_install: Pillow unavailable (pip install failed)" >&2
  exit 0
fi
if ! "$PY" -c 'import PIL' >/dev/null 2>&1; then
  echo "agent_env_install: Pillow unavailable (import still missing)" >&2
  exit 0
fi
"$PY" -c 'import PIL; print("Pillow", PIL.__version__)'
