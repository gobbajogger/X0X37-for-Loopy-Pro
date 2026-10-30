#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
OUT=build/host/miami-adsr-smc
mkdir -p "$OUT"
sh host/build_host.sh --generate-only >"$OUT/generate.log" 2>&1 || { cat "$OUT/generate.log"; exit 1; }
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/breaks.c
firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c
firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c
"
DARWIN=
[ "$(uname -s)" != Darwin ] || DARWIN=-D_DARWIN_C_SOURCE
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
  -DX0X_HOST -DX0X_DEV_PADCAL=1 -DX0X_DEV_WOBBLE=1 -D_POSIX_C_SOURCE=200809L $DARWIN \
  -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -Iexperiments/wobble \
  tests/host/miami_adsr_smc_test.c $U -lm -o "$OUT/test"
"$OUT/test"
