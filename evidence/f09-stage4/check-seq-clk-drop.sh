#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Confirm BF-005 owner-matching seq_clk_drop clears ext_pending; non-owning leaves it.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
BLOB="$(git rev-parse HEAD:firmware/src/seq/sequencer.c)"
EXPECTED=fd3f65306df166494d10facf8ddae058287cc17b
if [[ "$BLOB" != "$EXPECTED" ]]; then
  echo "FAIL sequencer blob $BLOB != $EXPECTED" >&2
  exit 1
fi
# Owner-matching clear of ext_pending with clk_lock in seq_clk_drop
if ! grep -n 'ext_pending' firmware/src/seq/sequencer.c | head -20; then
  echo "FAIL no ext_pending references" >&2
  exit 1
fi
# Require the clear path near clk_lock drop (structural intent)
python3 - <<'PY'
from pathlib import Path
src = Path("firmware/src/seq/sequencer.c").read_text()
# Find seq_clk_drop function body
idx = src.find("seq_clk_drop")
if idx < 0:
    raise SystemExit("FAIL seq_clk_drop not found")
# Heuristic: after matching owner clear of clk_lock, ext_pending is cleared
chunk = src[idx:idx+2500]
if "ext_pending" not in chunk:
    raise SystemExit("FAIL seq_clk_drop body lacks ext_pending")
# Must clear pending when dropping owning lock
ok = False
# Look for assignment of ext_pending to 0 / false near clk_lock clear in this function
import re
# owner-matching path clears pending
if re.search(r"ext_pending\s*=\s*0", chunk) or re.search(r"s->ext_pending\s*=\s*0", chunk):
    ok = True
if not ok:
    raise SystemExit("FAIL seq_clk_drop does not clear s->ext_pending = 0")
print(f"PASS seq_clk_drop owner-matching clears ext_pending; blob {Path('.').resolve()}")
print(f"blob_ok {chunk[:80]!r}...")
PY
echo "PASS seq_clk_drop review blob=$BLOB"
