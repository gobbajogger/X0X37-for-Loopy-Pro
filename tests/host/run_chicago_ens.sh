#!/bin/sh
# Chicago Ens insert — multi-voice detune host proof.
set -eu
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${ROOT}/build/chicago_ens"
mkdir -p "$OUT"
CC="${CC:-cc}"
CFLAGS="-std=c99 -O2 -Wall -Wextra -ffp-contract=off -I${ROOT}/firmware/src -I${ROOT}/firmware/src/dsp -I${ROOT}/firmware/src/app"
$CC $CFLAGS tests/host/chicago_ens_test.c firmware/src/app/chicago_param_tables.c -lm \
  -o "$OUT/chicago_ens"
"$OUT/chicago_ens"
