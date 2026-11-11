#!/bin/sh
# Build offline native303 renderer for Chicago AU battery.
set -eu
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${ROOT}/build/chicago_au"
mkdir -p "$OUT"
CC="${CC:-cc}"
CFLAGS="-std=c99 -O2 -Wall -Wextra -ffp-contract=off -Ifirmware/src -Ifirmware/src/dsp -Ifirmware/src/app"
# Match AU bounce + scenario manifests (48 kHz); device firmware stays 44.1 kHz.
# Ring insert (chicago_ring.h) is header-only; param tables not required for the renderer.
$CC $CFLAGS -DBASS303_SR=48000.0f tests/host/chicago_au_render.c -lm -o "$OUT/chicago_au_render"
echo "ok chicago_au_render -> $OUT/chicago_au_render"
