#!/bin/sh
# Replay independently authored edge checks against production source; no device I/O.
set -eu
cd "$(dirname "$0")/../.."
OUT=${MUTED_REVIEW_OUT:-build/host/muted-fader-independent}
# Generates current extracted keyboard/clip functions and real-engine accessor TU,
# and first runs the maintained matrix. The independently authored C probe is frozen.
SMC_MUTED_OUT="$OUT" sh tests/host/smc_muted_fader_test.sh
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c
firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c
firmware/src/seq/pattern.c firmware/src/seq/motion.c"
DARWIN=
[ "$(uname -s)" != Darwin ] || DARWIN=-D_DARWIN_C_SOURCE
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
 -DX0X_HOST $DARWIN ${SAN_FLAGS:-} -DSMC_SOURCE_FILE=\"$PWD/firmware/src/app/smc_mixer.c\" \
 -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Itests/host -Ibuild/gen -I"$OUT" \
 evidence/muted-fader-20261025/independent-extra.c "$OUT/engine.c" $U -lm -o "$OUT/independent-extra"
"$OUT/independent-extra"
