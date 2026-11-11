#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Owner display order: real UI/panel/SMC mapping, stable data IDs and rendered screens.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/part_order_test.c
export SMC_UI_OUT=${1:-build/host/part-order}
export X0X_HOST_DEFS="${X0X_HOST_DEFS:-} -DX0X_WOBBLE=1 -DX0X_DEV_WOBBLE=1 -DX0X_BASS_PRESET_PREVIEW=1 -DX0X_BASS_TAGGED_SOUND=1 -DX0X_BASS_NOTE_SOUND=0"
mkdir -p "$SMC_UI_OUT"
awk '/^static void smk_faders\(void\)/{on=1} on{print} on && /^}/{exit}' firmware/src/app/main_fm1.c > "$SMC_UI_OUT/smk_faders.inc"
test -s "$SMC_UI_OUT/smk_faders.inc"
exec sh tests/host/smc_ui_test.sh
