#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Confirm BF-007 project.c blob and static vs lane BD PAN migration order.
# Prefer the private worktree cwd from tools/dev_validate.py; fall back to repo root.
set -euo pipefail
if [[ -f firmware/src/app/project.c ]]; then
  ROOT="$PWD"
else
  ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
  cd "$ROOT"
fi
BLOB="$(git rev-parse HEAD:firmware/src/app/project.c)"
EXPECTED=240a4b1735166dfb990b52c26686c7ea5ad4317e
if [[ "$BLOB" != "$EXPECTED" ]]; then
  echo "FAIL project.c blob $BLOB != $EXPECTED" >&2
  exit 1
fi
python3 - <<'PY'
from pathlib import Path
import re
src = Path("firmware/src/app/project.c").read_text()
# static helper remaps sound.v only
m_static = re.search(
    r"static void sound_bd_sends\(.*?\n\}",
    src, re.S)
if not m_static:
    raise SystemExit("FAIL sound_bd_sends definition missing")
body = m_static.group(0)
if "pr->sound.v" not in body:
    raise SystemExit("FAIL sound_bd_sends must touch static sound.v")
if "arr.lane" in body or "sound_bd_sends_lanes" in body:
    raise SystemExit("FAIL sound_bd_sends must not touch lanes")
m_lanes = re.search(
    r"static void sound_bd_sends_lanes\(.*?\n\}",
    src, re.S)
if not m_lanes:
    raise SystemExit("FAIL sound_bd_sends_lanes definition missing")
lanes = m_lanes.group(0)
if "arr.lane" not in lanes and "pr->arr.lane" not in lanes:
    raise SystemExit("FAIL sound_bd_sends_lanes must walk arrangement lanes")
# call-order: static after OBJ_SOUND; lanes after arrange
obj = src.find("OBJ_SOUND")
static_call = src.find("sound_bd_sends(pr,", obj)
lane_call = src.find("sound_bd_sends_lanes(pr,", obj)
arrange = src.find("load_arrange1", obj)
if obj < 0 or static_call < 0 or lane_call < 0 or arrange < 0:
    raise SystemExit("FAIL missing OBJ_SOUND / migration call sites / load_arrange1")
if not (obj < static_call < arrange < lane_call):
    raise SystemExit(
        f"FAIL call order obj={obj} static={static_call} arrange={arrange} lanes={lane_call}"
    )
print("PASS BF-007 migration order: static post-OBJ_SOUND; lanes post-arrange")
PY
echo "PASS BF-007 project.c review blob=$BLOB"
