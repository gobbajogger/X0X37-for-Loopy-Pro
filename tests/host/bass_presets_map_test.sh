#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1 || { cat "$OUT/assets.log"; exit 1; }
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c"
for WBL in 0 1; do
 ${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter ${SAN_FLAGS:-} \
  -DX0X_HOST -D_DARWIN_C_SOURCE -D_POSIX_C_SOURCE=200809L -DX0X_WOBBLE=$WBL -DX0X_DEV_WOBBLE=$WBL \
  -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/bass_presets_map_test.c $U -lm -o "$OUT/probe-$WBL"
 "$OUT/probe-$WBL" "$@"
done
