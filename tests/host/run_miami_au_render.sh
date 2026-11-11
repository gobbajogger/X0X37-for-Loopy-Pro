#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${MIAMI_AU_RENDER_OUT:-${ROOT}/build/miami_au}"   # miami-au-battery builds a private copy (MIAMI_AU_RENDER_OUT)
mkdir -p "$OUT"
CC="${CC:-cc}"
CFLAGS="-std=c99 -O2 -Wall -Wextra -Werror -ffp-contract=off -I${ROOT}/firmware/src"
# ld recreates its -o file mode 0644 and sets +x only at the end: exec in that window is EACCES.
# Link to a private name, then rename, so a parallel reader sees an old or a new whole binary.
TMP="$OUT/.wobble_au_render.$$"
trap 'rm -f "$TMP"' EXIT
$CC $CFLAGS tests/host/wobble_au_render.c experiments/wobble/wobble.c -lm -o "$TMP"
mv -f "$TMP" "$OUT/wobble_au_render"
echo "ok wobble_au_render -> $OUT/wobble_au_render"
