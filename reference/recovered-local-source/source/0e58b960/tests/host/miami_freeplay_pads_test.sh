#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Actual SMK digital pad path + UI, engine and sequencer; no device writes.
set -eu
cd "$(dirname "$0")/../.."
OUTDIR=${X0X_PAD_TEST_OUT:-$(mktemp -d)}
if [ -z "${X0X_PAD_TEST_OUT:-}" ]; then trap 'rm -rf "$OUTDIR"' EXIT HUP INT TERM; fi
mkdir -p "$OUTDIR"
OUTDIR=$(cd "$OUTDIR" && pwd)
sh host/build_host.sh --generate-only > "$OUTDIR/generate.log" 2>&1
X0X_PAD_TEST_OUT="$OUTDIR" python3 tests/host/miami_freeplay_pads_fixture.py
OUT="$OUTDIR/probe"
cc ${SAN_FLAGS:-} -O1 -g -ffp-contract=off -std=c99 -w -DX0X_HOST -DX0X_PAD_DIV_TEST=1 -DX0X_WOBBLE=1 -DX0X_DEV_WOBBLE=1 -DX0X_BASS_PRESET_PREVIEW=1 -DX0X_BASS_TAGGED_SOUND=1 -DX0X_BASS_NOTE_SOUND=0 -DBOARD_SMK37=1 -DX0X_DEV_PADCAL=1 -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -o "$OUT" "$OUTDIR/probe.c" firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c experiments/wobble/wobble.c firmware/src/app/factory_banks.c -lm
X0X_PAD_TEST_OUT="$OUTDIR" "$OUT"
