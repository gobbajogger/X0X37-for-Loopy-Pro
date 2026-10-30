#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host proof for Chicago pitch bend + damper aux pots.
set -e
cd "$(dirname "$0")/../.."
OUT=build/chicago_aux
mkdir -p "$OUT"
CC="${CC:-cc}"
CFLAGS="-std=c99 -O2 -Wall -Wextra -Wno-unused-function -lm"
$CC $CFLAGS -I firmware/src -I firmware/src/app -I firmware/src/dsp \
  tests/host/chicago_aux_test.c firmware/src/app/chicago_param_tables.c \
  -o "$OUT/chicago_aux" -lm
"$OUT/chicago_aux"
