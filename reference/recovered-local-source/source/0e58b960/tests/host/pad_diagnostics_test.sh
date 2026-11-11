#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
OUT="${X0X_PAD_DIAG_OUT:-build/pad-diagnostics}"
mkdir -p "$OUT"
sh host/build_host.sh --generate-only > "$OUT/assets.log" 2>&1
awk '/^static void smk_wheels\(void\)/{on=1} on{print} on && /^}/{exit}' firmware/src/app/main_fm1.c > "$OUT/smk_wheels.inc"
awk '/^static void smk_faders\(void\)/{on=1} on{print} on && /^}/{exit}' firmware/src/app/main_fm1.c > "$OUT/smk_faders.inc"
awk '/^static void smk_arrows\(void\)/{on=1} on{print} on && /^}/{exit}' firmware/src/app/main_fm1.c > "$OUT/smk_arrows.inc"
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c firmware/src/app/factory_banks.c experiments/wobble/wobble.c"
${CC:-cc} ${CFLAGS:--O2 -g} -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -DBOARD_SMK37=1 -DX0X_LIVE_KEY_TEST=1 -DX0X_HOST -DX0X_SMK_CAPTIONS=1 -DX0X_WOBBLE=1 -DX0X_DEV_WOBBLE=1 -DX0X_BASS_PRESET_PREVIEW=1 -DX0X_BASS_TAGGED_SOUND=1 -I"$OUT" -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen tests/host/pad_diagnostics_test.c $U -lm -o "$OUT/pad-test"
"$OUT/pad-test" "$OUT"
