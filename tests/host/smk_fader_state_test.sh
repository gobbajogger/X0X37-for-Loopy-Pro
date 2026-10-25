#!/bin/sh
# Production keyboard scanner + real UI, engine, motion and sound parameters. No device I/O.
set -eu
cd "$(dirname "$0")/../.."
OUT=${SMK_FADER_STATE_OUT:-build/host/smk-fader-state}
mkdir -p "$OUT"
SOURCE=${SMK_FADER_SOURCE:-firmware/src/app/main_fm1.c}
SMC_SOURCE=${SMC_UI_SOURCE_FILE:-$PWD/firmware/src/app/smc_mixer.c}
awk '/^static void smk_faders\(void\)/{on=1} on{print} on && /^}/{exit}' "$SOURCE" > "$OUT/smk_faders.inc"
awk '/^static const char CK_FLD_N\[\] = /{print}
     /^static uint32_t ck_page_on\(void\)$/ || /^int smk_ck_smc_(turn|nav)\(/{on=1}
     on{print} on && /^}/{on=0}' firmware/src/app/main_fm1.c > "$OUT/clip_smc.inc"
sh host/build_host.sh --generate-only > "$OUT/generate.log" 2>&1 || { cat "$OUT/generate.log"; exit 1; }
# Test-only accessor observes the real engine state after processing its queue.
printf '#include "%s/firmware/src/app/engine.c"\nfloat keyboard_test_mix_level(int p) { return mix_level[p]; }\n' "$PWD" > "$OUT/engine.c"
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c
firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c
firmware/src/seq/pattern.c firmware/src/seq/motion.c"
DARWIN=
[ "$(uname -s)" != Darwin ] || DARWIN=-D_DARWIN_C_SOURCE
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
 -DX0X_HOST $DARWIN ${SAN_FLAGS:-} -DSMC_SOURCE_FILE=\"$SMC_SOURCE\" -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" \
 tests/host/smk_fader_state_test.c "$OUT/engine.c" $U -lm -o "$OUT/test"
if [ "${SMK_FADER_MUTE_AUDIT:-0}" = 1 ]; then exec "$OUT/test" mute; fi
if [ "${SMK_FADER_COLD_ONLY:-0}" = 1 ]; then exec "$OUT/test" cold 0 0 100 206; fi
for bank in 0 1; do
 for strip in 0 1 2 3; do
  for raw in 0 100 127; do
   for frames in 0 199 200 206 1000; do
    for held in 0 1; do "$OUT/test" cold "$bank" "$strip" "$raw" "$frames" "$held"; done
   done
  done
 done
done
"$OUT/test"
