#!/bin/sh
# Build offline native303 renderer for Chicago AU battery.
set -eu
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${CHICAGO_AU_RENDER_OUT:-${ROOT}/build/chicago_au}"   # chicago-au-battery builds a private copy (CHICAGO_AU_RENDER_OUT)
mkdir -p "$OUT"
CC="${CC:-cc}"
CFLAGS="-std=c99 -O2 -Wall -Wextra -ffp-contract=off -Ifirmware/src -Ifirmware/src/dsp -Ifirmware/src/app"
# Match AU bounce + scenario manifests (48 kHz); device firmware stays 44.1 kHz.
# Ring insert (chicago_ring.h) is header-only; param tables not required for the renderer.
# ld recreates its -o file mode 0644 and sets +x only at the end: exec in that window is EACCES.
# Link to a private name, then rename, so a parallel reader sees an old or a new whole binary.
TMP="$OUT/.chicago_au_render.$$"
trap 'rm -f "$TMP"' EXIT
$CC $CFLAGS -DBASS303_SR=48000.0f tests/host/chicago_au_render.c firmware/src/dsp/bass303.c -lm -o "$TMP"
mv -f "$TMP" "$OUT/chicago_au_render"
echo "ok chicago_au_render -> $OUT/chicago_au_render"
