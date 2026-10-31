#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Stage-4 BF-004 static gate: USB/BT Note Off uses src_held; TRS path still releases.
set -euo pipefail
ENG=firmware/src/app/engine.c
test -f "$ENG"
# Ownership gate present for src < 2 (Note Off and break-off paths).
grep -n 'src < 2' "$ENG" | head -20
grep -n 'src_held' "$ENG" | head -40
# Confirm blob identity for this worktree matches BF-004 repair.
BLOB=$(git rev-parse HEAD:firmware/src/app/engine.c)
EXPECTED=ab6d9667e9cd40ae12cc8c8ece8d7bb12be29216
test "$BLOB" = "$EXPECTED"
echo "engine_blob_ok $BLOB"
echo "noteoff_gate_ok"
