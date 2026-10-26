#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${ROOT}/build/miami_au"
mkdir -p "$OUT"
CC="${CC:-cc}"
CFLAGS="-std=c99 -O2 -Wall -Wextra -Werror -ffp-contract=off -I${ROOT}/firmware/src"
$CC $CFLAGS tests/host/wobble_au_render.c experiments/wobble/wobble.c -lm -o "$OUT/wobble_au_render"
echo "ok wobble_au_render -> $OUT/wobble_au_render"
