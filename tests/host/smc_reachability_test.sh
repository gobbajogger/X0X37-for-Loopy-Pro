#!/bin/sh
# Run production UI/project/engine/SMC together. Generates only scratch output.
set -eu
cd "$(dirname "$0")/../.."
SOURCE=${SMC_SOURCE_ROOT:-$PWD}
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
awk '/^static const char CK_FLD_N\[\] = /{print}
     /^static uint32_t ck_page_on\(void\)$/ || /^int smk_ck_smc_(turn|nav)\(/{on=1}
     on{print} on && /^}/{on=0}' "$SOURCE/firmware/src/app/main_fm1.c" > "$OUT/clip_smc.inc"
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c
firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c
firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c"
# The supported host builder generates the same fonts/sample tables first.
X0X_HOST_OUT="$OUT/host" sh host/build_host.sh --generate-only > "$OUT/build.log" 2>&1 || { cat "$OUT/build.log"; exit 1; }
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -DX0X_HOST -DSMC_SOURCE_FILE=\"$SOURCE/firmware/src/app/smc_mixer.c\" ${X0X_HOST_DEFS:-} -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" \
    tests/host/smc_reachability_test.c $U -lm -o "$OUT/test"
"$OUT/test"
