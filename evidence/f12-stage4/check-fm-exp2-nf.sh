#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Confirm BF-008 fm_exp2_nf uses the defined unsigned exponent shift (blob a5c26c95...).
# Prefer the private worktree cwd from tools/dev_validate.py; fall back to repo root.
set -euo pipefail
if [[ -f firmware/src/dsp/fastmath.h ]]; then
  ROOT="$PWD"
else
  ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
  cd "$ROOT"
fi
BLOB="$(git rev-parse HEAD:firmware/src/dsp/fastmath.h)"
EXPECTED=a5c26c95cad0d91a548a8db073f59777e36d4e86
if [[ "$BLOB" != "$EXPECTED" ]]; then
  echo "FAIL fastmath.h blob $BLOB != $EXPECTED" >&2
  exit 1
fi
python3 - <<'PY'
from pathlib import Path
src = Path("firmware/src/dsp/fastmath.h").read_text()
idx = src.find("fm_exp2_nf")
if idx < 0:
    raise SystemExit("FAIL fm_exp2_nf not found")
chunk = src[idx:idx+1200]
needle = "b.u += (uint32_t)n << 23;"
if needle not in chunk:
    raise SystemExit("FAIL defined unsigned exponent shift missing in fm_exp2_nf")
if "b.u += n << 23;" in chunk.replace(needle, ""):
    raise SystemExit("FAIL signed shift pattern still present beside defined form")
print("PASS fm_exp2_nf defined unsigned shift present")
PY
echo "PASS fm_exp2_nf review blob=$BLOB"
