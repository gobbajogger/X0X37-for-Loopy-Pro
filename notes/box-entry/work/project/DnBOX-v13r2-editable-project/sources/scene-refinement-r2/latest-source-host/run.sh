#!/bin/sh
set -eu
SRC=${DNBOX_FIRMWARE_SOURCE:?Set DNBOX_FIRMWARE_SOURCE to exact source checkout}
OUT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SRC"
test "$(git rev-parse HEAD^{tree})" = 533564a4085bace9e65245b45affbc45c54a647e
test -z "$(git status --porcelain)"
U='firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c firmware/src/app/factory_banks.c experiments/wobble/wobble.c'
cc -O1 -g -ffp-contract=off -std=c99 -Wno-unused-function -Wno-unused-parameter -DX0X_HOST -D_POSIX_C_SOURCE=200809L -DX0X_WOBBLE=1 -DX0X_DEV_WOBBLE=1 -DX0X_BASS_PRESET_PREVIEW=1 -DX0X_BASS_TAGGED_SOUND=1 -DX0X_BASS_NOTE_SOUND=0 -DSMC_SOURCE_FILE=\"$SRC/firmware/src/app/smc_mixer.c\" -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -I"$OUT" -I"$SRC/tests/host" -Ibuild/gen "$OUT/preview_pages.c" $U -lm -o "$OUT/preview-pages"
SCOPE_PCM="$OUT/common-first-bar-scope128.pcm" SMC_UI_SHOTS="$OUT" "$OUT/preview-pages"
test -z "$(git status --porcelain)"
