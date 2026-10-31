#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${X0X_LIVE_TEST_OUT:-build/host/freeplay-live-keys}
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
sh host/build_host.sh --generate-only >"$OUT/assets.log" 2>&1
X0X_LIVE_TEST_OUT="$OUT" python3 tests/host/freeplay_live_keys_fixture.py
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c firmware/src/app/factory_banks.c experiments/wobble/wobble.c"
${CC:-cc} ${SAN_FLAGS:-} -O1 -g -ffp-contract=off -std=c99 -w -DX0X_HOST -DX0X_LIVE_KEY_TEST=1 -DX0X_WOBBLE=1 -DX0X_DEV_WOBBLE=1 -DX0X_BASS_PRESET_PREVIEW=1 -DX0X_BASS_TAGGED_SOUND=1 -DX0X_BASS_NOTE_SOUND=0 -DBOARD_SMK37=1 -DX0X_DEV_PADCAL=1 -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen "$OUT/probe.c" $U -lm -o "$OUT/test"
"$OUT/test" "$OUT"
