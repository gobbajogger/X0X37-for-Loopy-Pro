#!/bin/sh
# Chicago Crush/Decimator insert — quantize/hold host proof.
set -eu
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${ROOT}/build/chicago_crush"
mkdir -p "$OUT"
CC="${CC:-cc}"
CFLAGS="-std=c99 -O2 -Wall -Wextra -ffp-contract=off -I${ROOT}/firmware/src -I${ROOT}/firmware/src/dsp -I${ROOT}/firmware/src/app"
$CC $CFLAGS tests/host/chicago_crush_test.c firmware/src/app/chicago_param_tables.c -lm \
  -o "$OUT/chicago_crush"
"$OUT/chicago_crush"
