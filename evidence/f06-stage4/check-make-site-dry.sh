#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# BF-003 Stage 4 host check: make_site dry on synthetic FM-1_900.
set -euo pipefail
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
PKG="$TMP/syn.fwsc"
OUT="$TMP/out"
python3 - "$PKG" <<'PY'
import sys
from pathlib import Path
BLOCKS, BLK, KEEP = 20, 0x30, 0x2F
product = "FM-1_900"
raw = bytearray(BLOCKS * BLK + 64)
for i in range(BLOCKS):
    raw[i * BLK + KEEP] = 0x7D
for i, ch in enumerate(product):
    raw[i * BLK + KEEP] = (ord(ch) + i + 1) & 0xFF
# Keep FELUCCA-LOADER-1 after the product marker slots (i*BLK+KEEP).
off = BLOCKS * BLK + 16
raw[off:off + len(b"FELUCCA-LOADER-1")] = b"FELUCCA-LOADER-1"
Path(sys.argv[1]).write_bytes(raw)
PY
mkdir -p "$OUT/webapp/editor"
echo stale > "$OUT/webapp/editor/index.html"
python3 web/make_site.py "$PKG" 0.0-bf003-s4 "$OUT"
test -f "$OUT/webapp/installer/index.html"
test ! -e "$OUT/webapp/editor"
test ! -e web/editor.html
echo "make_site_dry_ok installer_only no_editor_copy"
