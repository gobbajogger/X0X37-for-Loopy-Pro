#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Confirm BF-006 d8_exp2_cr uses the defined exponent bias shift (blob 90f46329...).
# Prefer the private worktree cwd from tools/dev_validate.py; fall back to repo root.
set -euo pipefail
if [[ -f firmware/src/dsp/drum808.c ]]; then
  ROOT="$PWD"
else
  ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
  cd "$ROOT"
fi
BLOB="$(git rev-parse HEAD:firmware/src/dsp/drum808.c)"
EXPECTED=90f463295fb1a642bad81700d73fc22eb8ec7a38
if [[ "$BLOB" != "$EXPECTED" ]]; then
  echo "FAIL drum808 blob $BLOB != $EXPECTED" >&2
  exit 1
fi
python3 - <<'PY'
from pathlib import Path
import re
src = Path("firmware/src/dsp/drum808.c").read_text()
idx = src.find("d8_exp2_cr")
if idx < 0:
    raise SystemExit("FAIL d8_exp2_cr not found")
chunk = src[idx:idx+1200]
needle = "b.u += (uint32_t)(int32_t)n << 23;"
if needle not in chunk:
    raise SystemExit("FAIL defined exponent bias shift missing in d8_exp2_cr")
if not re.search(r"if\s*\(\s*x\s*<\s*-126\.0f\s*\)\s*return\s+0\.0f\s*;", chunk):
    raise SystemExit("FAIL lower clamp missing")
if not re.search(r"if\s*\(\s*x\s*>\s*127\.0f\s*\)\s*x\s*=\s*127\.0f\s*;", chunk):
    raise SystemExit("FAIL upper clamp missing")
print("PASS d8_exp2_cr defined shift present; clamps intact")
PY
echo "PASS d8_exp2_cr review blob=$BLOB"
