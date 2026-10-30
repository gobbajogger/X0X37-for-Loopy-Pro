#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host proof: Chicago AU Env → filter EG dual-drive + Gate DSP path.
set -e
cd "$(dirname "$0")/../.."
OUT=build/chicago_env
mkdir -p "$OUT"
CC="${CC:-cc}"
CFLAGS="-std=c99 -O2 -Wall -Wextra -Wno-unused-function -DBASS303_SR=48000.0f"
$CC $CFLAGS -I firmware/src -I firmware/src/app -I firmware/src/dsp \
  tests/host/chicago_env_test.c -o "$OUT/chicago_env" -lm
"$OUT/chicago_env"
