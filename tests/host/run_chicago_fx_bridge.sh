#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host proof for Chicago SEND FX Type/Edit bridge (Mix/fxbus approximation).
set -e
cd "$(dirname "$0")/../.."
OUT=build/chicago_fx_bridge
mkdir -p "$OUT"
CC="${CC:-cc}"
CFLAGS="-std=c99 -O2 -Wall -Wextra -Wno-unused-function"
$CC $CFLAGS -I firmware/src -I firmware/src/app -I firmware/src/dsp \
  tests/host/chicago_fx_bridge_test.c firmware/src/app/chicago_param_tables.c \
  -o "$OUT/chicago_fx_bridge"
"$OUT/chicago_fx_bridge"
